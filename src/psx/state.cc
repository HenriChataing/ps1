
#include <mutex>

#include <psx/debugger.h>
#include <psx/psx.h>
#include <psx/hw.h>

#include <assembly/disassembler.h>
#include <interpreter/interpreter.h>

using namespace psx;

namespace psx {

/// State declaration.
struct state state;

state::state() {
    bus = new psx::DefaultBus();
}

state::~state() {
    delete bus;
}

int state::load_bios(std::istream &bios_contents) {
    memset(bios, 0, sizeof(bios));
    bios_contents.read((char *)bios, sizeof(bios));
    return bios_contents.gcount() > 0 ? 0 : -1;
}

int state::load_cd_rom(std::istream &cd_rom_contents) {
    if (cd_rom != NULL) {
        delete cd_rom;
    }

    cd_rom_contents.seekg(0, cd_rom_contents.end);
    cd_rom_size = cd_rom_contents.tellg();
    cd_rom_contents.seekg(0, cd_rom_contents.beg);
    cd_rom = new uint8_t[cd_rom_size];
    cd_rom_contents.read((char *)cd_rom, cd_rom_size);
    return 0;
}

void state::reset() {
    // Clear the machine state.
    memset(ram, 0, sizeof(ram));
    memset(dram, 0, sizeof(dram));
    cpu = (psx::cpu_registers){};
    cp0 = (psx::cp0_registers){};
    hw = (psx::hw_registers){};

    cancel_all_events();
    hw::hblank_event();

    // Set the register reset values.
    cpu.pc   = UINT32_C(0xbfc00000);
    cp0.prid = UINT32_C(0x00000002);
    cp0.sr   = STATUS_BEV | STATUS_TS;

    hw.dpcr     = UINT32_C(0x07654321);
    hw.joy_stat = UINT32_C(0x00000005);
    cdrom.index = UINT8_C(0x18);

    // Setup initial action.
    cycles = 0;
    cpu_state = psx::Jump;
    jump_address = cpu.pc;
}

void state::schedule_event(unsigned long timeout, void (*callback)()) {
    std::lock_guard<std::mutex> lock(_mutex);
    struct event *event = new psx::event(timeout, callback, NULL);
    struct event *pos = event_queue, **prev = &event_queue;
    while (pos != NULL) {
        unsigned long udiff = timeout - pos->timeout;
        if (udiff > std::numeric_limits<long long>::max())
            break;
        prev = &pos->next;
        pos = pos->next;
    }
    *prev = event;
    event->next = pos;
    next_event = event_queue->timeout;
}

void state::cancel_event(void (*callback)()) {
    std::lock_guard<std::mutex> lock(_mutex);
    struct event *pos = event_queue, **prev = &event_queue;
    while (pos != NULL) {
        if (pos->callback == callback) {
            struct event *tmp = pos;
            *prev = pos->next;
            pos = pos->next;
            delete tmp;
        } else {
            prev = &pos->next;
            pos = pos->next;
        }
    }
}

void state::cancel_all_events(void) {
    std::lock_guard<std::mutex> lock(_mutex);
    struct event *pos = event_queue;
    event_queue = NULL;
    next_event = -1lu;
    while (pos != NULL) {
        struct event *tmp = pos;
        pos = pos->next;
        delete tmp;
    }
}

void state::handle_event(void) {
    std::unique_lock<std::mutex> lock(_mutex);
    if (next_event > cycles) {
        // false positive, no event actually scheduled to be
        // executed right now.
        return;
    }
    if (event_queue != NULL) {
        struct event *event = event_queue;
        event_queue = event->next;
        // Unlock now: the callback function may re-schedule an event
        // and may need to re-take the lock for this purpose.
        lock.unlock();
        event->callback();
        delete event;
    }
    next_event = (event_queue != NULL) ? event_queue->timeout : cycles - 1;
}

/**
 * @brief Update the count register.
 *  The count register increments at half the CPU frequency.
 *  If the value of the Count register equals that of the Compare
 *  register, set the IP7 bit of the Cause register.
 */
void handle_counter_event() {
    // Note: the interpreter being far from cycle exact,
    // the Count register value will necessary be inexact.

#if 0
    unsigned long diff = (state.cycles - state.cp0.lastCounterUpdate) / 2;
    uint32_t untilCompare = state.cp0.compare - state.cp0.count;

    debugger::debug(Debugger::COP0, "counter event");
    debugger::debug(Debugger::COP0, "  count:{}", state.cp0.count);
    debugger::debug(Debugger::COP0, "  compare:{}", state.cp0.compare);
    debugger::debug(Debugger::COP0, "  cycles:{}", state.cycles);
    debugger::debug(Debugger::COP0, "  last_counter_update:{}", state.cp0.lastCounterUpdate);

    if (diff >= untilCompare) {
        state.cp0.cause |= CAUSE_IP7;
        check_interrupt();
    } else {
        psx::halt("Spurious counter event");
    }

    state.cp0.count = (uint32_t)((unsigned long)state.cp0.count + diff);
    state.cp0.lastCounterUpdate = state.cycles;
    untilCompare = state.cp0.compare - state.cp0.count - 1;

    debugger::debug(Debugger::COP0, "  now:{}", state.cycles);
    debugger::debug(Debugger::COP0, "  trig:{}", state.cycles + 2 * (unsigned long)untilCompare);
    state.scheduleEvent(state.cycles + 2 * (unsigned long)untilCompare, handleCounterEvent);
#endif
}

/**
 * Called to reconfigure the counter event in the case either the Compare
 * of the Counter register is written.
 */
void schedule_counter_event() {
#if 0
    unsigned long diff = (state.cycles - state.cp0.lastCounterUpdate) / 2;
    state.cp0.count = (uint32_t)((unsigned long)state.cp0.count + diff);
    state.cp0.lastCounterUpdate = state.cycles;
    uint32_t untilCompare = state.cp0.compare - state.cp0.count;

    debugger::debug(Debugger::COP0, "scheduling counter event");
    debugger::debug(Debugger::COP0, "  now:{}", state.cycles);
    debugger::debug(Debugger::COP0, "  trig:{}", state.cycles + 2 * (unsigned long)untilCompare);
    state.cancelEvent(handleCounterEvent);
    state.scheduleEvent(state.cycles + 2 * (unsigned long)untilCompare, handleCounterEvent);
#endif
}

/**
 * @brief Check whether an interrupt exception is raised from the current state.
 *  Take the interrupt exception if this is the case.
 */
void check_interrupt(void) {
    // For the interrupt to be taken, the interrupts must globally enabled
    // (IEc = 1) and the particular interrupt must be unmasked (IM[irq] = 1).
    if (state.cp0.IEc() && (state.cp0.IM() & state.cp0.IP()) != 0) {
        // Arrange for the interrupt to be taken at the following instruction :
        // The present instruction which enabled the interrupt must
        // not be repeated.
        //
        // Two cases here :
        // 1. called from instruction eval function,
        //    check next action to determine the following instruction.
        // 2. called from event handler. The result is the same, event
        //    handlers are always called before the instruction to execute
        //    is determined.
        switch (state.cpu_state) {
        case psx::Continue:
            state.cpu.pc += 4;
            state.delay_slot = false;
            break;

        case psx::Delay:
            state.cpu.pc += 4;
            state.delay_slot = true;
            break;

        case psx::Jump:
            state.cpu.pc = state.jump_address;
            state.delay_slot = false;
            break;
        }

        take_exception(Interrupt, 0, false, false, 0);
    }
}

/**
 * @brief Set the selected interrupt pending bit in the Cause register.
 *  The Interrupt exception will be taken just before executing the next
 *  instruction if the conditions are met (see \ref check_interrupt).
 *
 * @param irq           Interrupt number.
 */
void set_interrupt_pending(unsigned irq) {
    // Update the pending bits in the Cause register.
    state.cp0.cause |= CAUSE_IP(1lu << irq);
    check_interrupt();
}

void clear_interrupt_pending(unsigned irq) {
    // Update the pending bits in the Cause register.
    state.cp0.cause &= ~CAUSE_IP(1lu << irq);
}

/**
 * @brief Raise an exception and update the state of the processor.
 *  The delay slot parameter is provided by the state member cpu.delaySlot.
 *
 * @param vAddr
 *      Virtual address being accessed. Required for AddressError,
 *      TLBRefill, XTLBRefill, TLBInvalid, TLBModified,
 *      VirtualCoherency exceptions.
 * @param instr
 *      Whether the exception was triggered by an instruction fetch.
 * @param load
 *      Whether the exception was triggered by a load or store operation.
 * @param ce
 *      Index of the coprocessor for CoprocessorUnusable exceptions.
 */
void take_exception(enum cpu_exception exn, uint32_t vaddr, bool instr,
                    bool load, uint32_t ce)
{
    // Default vector valid for all general exceptions.
    uint32_t vector = UINT32_C(0x80);
    uint32_t exccode = 0;

    // Following the diagrams printed in the following section of the
    // reference manual:
    //      5.4 Exception Handling and Servicing Flowcharts,

    // Code specific to each exception.
    // The exception code and vector is determined by the exception type.
    // Each exception may set various register fields.
    switch (exn) {
        // The Address Error exception occurs when an attempt is made to execute
        // one of the following:
        //      - load or store from/to an unaligned memory location.
        //          (e.g. LW from an address that is not aligned to a word)
        //      - reference the kernel address space from User or Supervisor
        //          mode
        //      - reference the supervisor address space from User mode
        case AddressError:
            exccode = load ? 4 : 5; // AdEL : AdES
            state.cp0.badvaddr = vaddr;
            debugger::error(Debugger::CPU,
                "exception AddressError({:08x},{})",
                vaddr, load);
            psx::halt("AddressError");
            break;
        // TLB Refill occurs when there is no TLB entry that matches an
        // attempted reference to a mapped address space.
        case TLBRefill:         psx::halt("TLBRefill"); break;
        case TLBInvalid:        psx::halt("TLBInvalid"); break;
        // TLB Modified occurs when a store operation virtual address
        // reference to memory matches a TLB entry which is marked
        // valid but is not dirty (the entry is not writable).
        case TLBModified:       psx::halt("TLBModified"); break;
        // The Cache Error exception occurs when either a secondary cache ECC
        // error, primary cache parity error, or SysAD bus parity/ECC error
        // condition occurs and error detection is enabled.
        case CacheError:        psx::halt("CacheError"); break;
        // A Virtual Coherency exception occurs when all of the following
        // conditions are true:
        //      - a primary cache miss hits in the secondary cache
        //      - bits 14:12 of the virtual address were not equal to the
        //        corresponding bits of the PIdx field of the secondary
        //        cache tag
        //      - the cache algorithm for the page (from the C field in the TLB)
        //        specifies that the page is cached
        case VirtualCoherency:  psx::halt("VirtualCoherency"); break;
        // A Bus Error exception is raised by board-level circuitry for events
        // such as bus time-out, backplane bus parity errors, and invalid
        // physical memory addresses or access types.
        case BusError:
            exccode = instr ? 6 : 7; // IBE : DBE
            debugger::info(Debugger::CPU, "exception BusError({})", instr);
            psx::halt("BusError");
            break;
        // An Integer Overflow exception occurs when an ADD, ADDI, SUB, DADD,
        // DADDI or DSUB instruction results in a 2’s complement overflow
        case IntegerOverflow:
            exccode = 12; // Ov
            debugger::info(Debugger::CPU, "exception IntegerOverflow");
            psx::halt("IntegerOverflow");
            break;
        // A System Call exception occurs during an attempt to execute the
        // SYSCALL instruction.
        case SystemCall:
            exccode = 8; // Sys
            debugger::info(Debugger::CPU, "exception SystemCall");
            break;
        // A Breakpoint exception occurs when an attempt is made to execute the
        // BREAK instruction.
        case Breakpoint:
            exccode = 9; // Bp
            debugger::info(Debugger::CPU, "exception Breakpoint");
            psx::halt("Breakpoint");
            break;
        // The Reserved Instruction exception occurs when one of the following
        // conditions occurs:
        //      - an attempt is made to execute an instruction with an undefined
        //        major opcode (bits 31:26)
        //      - an attempt is made to execute a SPECIAL instruction with an
        //        undefined minor opcode (bits 5:0)
        //      - an attempt is made to execute a REGIMM instruction with an
        //        undefined minor opcode (bits 20:16)
        //      - an attempt is made to execute 64-bit operations in 32-bit mode
        //        when in User or Supervisor mode
        case ReservedInstruction:
            exccode = 10; // RI
            debugger::info(Debugger::CPU, "exception ReservedInstruction");
            psx::halt("ReservedInstruction");
            break;
        // The Coprocessor Unusable exception occurs when an attempt is made to
        // execute a coprocessor instruction for either:
        //      - a corresponding coprocessor unit that has not been marked
        //        usable, or
        //      - CP0 instructions, when the unit has not been marked usable
        //        and the process executes in either User or Supervisor mode.
        case CoprocessorUnusable:
            exccode = 11; // CpU
            debugger::info(Debugger::CPU, "exception CoprocessorUnusable({})", ce);
            psx::halt("CoprocessorUnusable");
            break;
        // The Interrupt exception occurs when one of the eight interrupt conditions
        // is asserted.
        case Interrupt:
            exccode = 0;
            debugger::info(Debugger::CPU, "exception Interrupt");
            break;
        default:
            psx::halt("UndefinedException");
            break;
    }

    // Set Cause Register : EXCCode, CE
    state.cp0.cause &= ~(CAUSE_EXCCODE_MASK | CAUSE_CE_MASK);
    state.cp0.cause |= CAUSE_EXCCODE(exccode) | CAUSE_CE(ce);
    // Swap processor mode and interrupt enable bits.
    uint32_t ku_ie = state.cp0.sr & UINT32_C(0x3f);
    state.cp0.sr &= ~UINT32_C(0x3f);
    state.cp0.sr |= ku_ie << 2;

    // Check if the exception was caused by a delay slot instruction.
    // Set EPC and Cause:BD accordingly.
    if (state.delay_slot) {
        state.cp0.epc = state.cpu.pc - 4;
        state.cp0.cause |= CAUSE_BD;
    } else {
        state.cp0.epc = state.cpu.pc;
        state.cp0.cause &= ~CAUSE_BD;
    }

    // Check if exceuting bootstrap code
    // and jump to the designated handler.
    uint32_t pc;
    if (state.cp0.BEV()) {
        pc = UINT32_C(0xbfc00100) + vector;
    } else {
        pc = UINT32_C(0x80000000) + vector;
    }

    state.cpu_state = psx::Jump;
    state.jump_address = pc;
}

#define KSEG2  UINT32_C(0xc0000000)
#define KSEG1  UINT32_C(0xa0000000)
#define KSEG0  UINT32_C(0x80000000)
#define KUSEG  UINT32_C(0x00000000)

cpu_exception translate_address(
    uint32_t virt_addr, uint32_t *phys_addr,
    bool write_access, uint32_t *virt_start, uint32_t *virt_end)
{
    uint32_t vstart = 0;
    uint32_t vend = 0;

    if (virt_addr < KSEG0) {
        *phys_addr  = virt_addr;
        vstart = 0;
        vend   = KSEG0 - 1;
    } else if (state.cp0.KUc()) {
        return cpu_exception::AddressError;
    } else if (virt_addr < KSEG1) {
        *phys_addr  = virt_addr - KSEG0;
        vstart = KSEG0;
        vend   = KSEG1 - 1;
    } else if (virt_addr < KSEG2) {
        *phys_addr  = virt_addr - KSEG1;
        vstart = KSEG1;
        vend   = KSEG2 - 1;
    } else {
        *phys_addr  = virt_addr;
        vstart = KSEG2;
        vend   = UINT32_C(0xffffffff);
    }

    if (virt_start != NULL)
        *virt_start = vstart;
    if (virt_end != NULL)
        *virt_end = vend;

    //debugger::warn(Debugger::CPU, "translate_address: {:08x} -> {:08x}", virt_addr, *phys_addr);
    return cpu_exception::None;
}

}; /* namespace psx */
