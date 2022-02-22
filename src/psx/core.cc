
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

#include <fmt/color.h>
#include <fmt/format.h>

#include <interpreter.h>
#include <psx/psx.h>
#include <psx/debugger.h>

namespace psx {

static std::thread            *interpreter_thread;
static std::mutex              interpreter_mutex;
static std::condition_variable interpreter_semaphore;
static std::atomic_bool        interpreter_halted;
static std::atomic_bool        interpreter_stopped;
static std::string             interpreter_halted_reason;

/**
 * Handle scheduled events (counter timeout, VI interrupt).
 * Called only at block endings.
 */
static
void check_cpu_events(void) {
    if (state.cycles >= state.next_event) {
        state.handle_event();
    }
}

/**
 * Run the interpreter until the n-th branching instruction.
 * The loop is also brokn by setting the halted flag.
 * The state is left with action Jump.
 * @return true exiting because of a branch instruction, false if because
 *  of a breakpoint.
 */
static
bool exec_cpu_interpreter(int nr_jumps) {
    while (!interpreter_halted.load(std::memory_order_acquire)) {
        switch (state.cpu_state) {
        case psx::Continue:
            state.cpu.pc += 4;
            state.delay_slot = false;
            interpreter::cpu::eval();
            break;

        case psx::Delay:
            state.cpu.pc += 4;
            state.cpu_state = psx::Jump;
            state.delay_slot = true;
            interpreter::cpu::eval();
            break;

        case psx::Jump:
            if (nr_jumps-- <= 0) return true;
            state.cpu.pc = state.jump_address;
            state.cpu_state = psx::Continue;
            state.delay_slot = false;
            interpreter::cpu::eval();
        }
    }
    return false;
}

/**
 * @brief Interpreter thead routine.
 * Loops interpreting machine instructions.
 * The interpreter will run in three main modes:
 *  1. interpreter execution, until coming to a block starting point,
 *     and then when the recompilation hasn't been issued or completed.
 *  2. recompiled code execution, when available. Always starts at the
 *     target of a branching instruction, and stops at the next branch
 *     or synchronous exception.
 *  3. step-by-step execution from the GUI. The interpreter is stopped
 *     during this time, and instructions are executed from the main thread.
 */
static
void interpreter_routine(void) {
    fmt::print(fmt::fg(fmt::color::dark_orange),
        "interpreter thread starting\n");

    for (;;) {
        std::unique_lock<std::mutex> lock(interpreter_mutex);
        interpreter_semaphore.wait(lock, [] {
            return !interpreter_halted.load(std::memory_order_acquire) ||
                interpreter_stopped.load(std::memory_order_acquire); });

        if (interpreter_stopped.load(std::memory_order_acquire)) {
            fmt::print(fmt::fg(fmt::color::dark_orange),
                "interpreter thread exiting\n");
            return;
        }

        fmt::print(fmt::fg(fmt::color::dark_orange),
            "interpreter thread resuming\n");

        lock.unlock();
        exec_cpu_interpreter(0);

        while (!interpreter_halted.load(std::memory_order_relaxed)) {
            // Ensure that the interpreter is at a jump
            // at the start of the loop contents.
            check_cpu_events();
            exec_cpu_interpreter(1);
        }

        fmt::print(fmt::fg(fmt::color::dark_orange),
            "interpreter thread halting\n");
    }
}

void start(void) {
    if (interpreter_thread == NULL) {
        interpreter_halted = true;
        interpreter_halted_reason = "reset";
        interpreter_thread = new std::thread(interpreter_routine);
    }
}

void stop(void) {
    if (interpreter_thread != NULL) {
        interpreter_halted.store(true, std::memory_order_release);
        interpreter_stopped.store(true, std::memory_order_release);
        interpreter_semaphore.notify_one();
        interpreter_thread->join();
        delete interpreter_thread;
        interpreter_thread = NULL;
    }
}

void reset(void) {
    state.reset();
}

void halt(std::string reason) {
    if (!interpreter_halted) {
        interpreter_halted_reason = reason;
        interpreter_halted.store(true, std::memory_order_release);
    }
}

bool halted(void) {
    return interpreter_halted;
}

std::string halted_reason(void) {
    return interpreter_halted_reason;
}

void step(void) {
    if (interpreter_thread != NULL &&
        interpreter_halted.load(std::memory_order_acquire)) {
        check_cpu_events();
        exec_cpu_interpreter(1);
    }
}

void resume(void) {
    if (interpreter_thread != NULL &&
        interpreter_halted.load(std::memory_order_acquire)) {
        interpreter_halted.store(false, std::memory_order_release);
        interpreter_semaphore.notify_one();
    }
}

}; /* namespace psx */
