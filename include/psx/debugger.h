
#ifndef _DEBUGGER_H_INCLUDED_
#define _DEBUGGER_H_INCLUDED_

#include <map>
#include <fmt/format.h>
#include <fmt/color.h>

#include <lib/circular_buffer.h>

class Debugger {
public:
    Debugger();
    ~Debugger();

    enum Verbosity {
        None,
        Error,
        Warn,
        Info,
        Debug,
    };

    enum Label {
        CPU,
        COP0,
        COP2,
        MC,
        JC,
        IC,
        Timer,
        DMA,
        CDROM,
        GPU,
        LabelCount,
    };

    Verbosity verbosity[LabelCount];
    fmt::rgb color[LabelCount];

    /** Type of execution trace entries. */
    typedef std::pair<uint32_t, uint32_t> TraceEntry;

    circular_buffer<TraceEntry> cpu_trace;
    std::string config_file;

    struct Breakpoint {
        unsigned id;
        uint64_t addr;
        bool enabled;

        Breakpoint(unsigned id, uint64_t addr)
            : id(id), addr(addr), enabled(true) {}
        Breakpoint()
            : id(0), addr(0), enabled(false) {}
    };

    struct Watchpoint {
        unsigned id;
        uint64_t start_addr;
        uint64_t end_addr;
        bool enabled;

        Watchpoint(unsigned id, uint64_t start_addr, uint64_t end_addr)
            : id(id), start_addr(start_addr), end_addr(end_addr), enabled(true) {}
        Watchpoint()
            : id(0), start_addr(0), end_addr(0), enabled(false) {}
    };

    /**
     * @brief Create a new breakpoint.
     * @details The breakpoint is always created; the input address is not
     *  checked for duplicates. The breakpoint identifier is monotonic.
     * @param addr
     *      Physical address of the RAM memory location to set
     *      the breakpoint to.
     * @return The identifier of the created breakpoint.
     */
    unsigned set_breakpoint(uint64_t addr);
    /**
     * @brief Remove a previously created breakpoint.
     * @param id    Identifier of the breakpoint to remove.
     */
    void unset_breakpoint(unsigned id);
    /**
     * @brief Check if the input address triggers a breakpoint.
     * @param addr  Physical address to check.
     * @param id    Return the identifier of the triggered breakpoint.
     * @return True if and only if the address \p addr is marked by a breakpoint.
     */
    bool check_breakpoint(uint64_t addr, unsigned *id = NULL);

    /**
     * @brief Create a new watchpoint.
     * @details The watchpoint is always created; the input addresses are not
     *  checked for duplicates. The watchpoint identifier is monotonic.
     * @param start_addr
     *      Start physical address of the RAM memory location to set
     *      the watchpoint to.
     * @param end_addr
     *      End physical address of the RAM memory location to set
     *      the watchpoint to.
     * @return The identifier of the created watchpoint.
     */
    unsigned set_watchpoint(uint64_t start_addr, uint64_t end_addr);
    /**
     * @brief Remove a previously created watchpoint.
     * @param id    Identifier of the watchpoint to remove.
     */
    void unset_watchpoint(unsigned id);
    /**
     * @brief Check if the input address range triggers a breakpoint.
     * @param addr  Physical address to check.
     * @param id    Return the identifier of the triggered breakpoint.
     * @return True if and only if the address \p addr is marked by a breakpoint.
     */
    bool check_watchpoint(uint64_t start_addr, uint64_t end_addr,
        unsigned *id = NULL);

    /**
     * Load debugger settings from save file.
     */
    void load_settings(std::string config_file = "");

    /**
     * Save debugger settings to save file provided
     * to \ref load_settings.
     */
    void save_settings();

    std::map<unsigned, Debugger::Breakpoint>::iterator breakpointsBegin() {
        return _breakpoints.begin();
    }

    std::map<unsigned, Debugger::Breakpoint>::iterator breakpointsEnd() {
        return _breakpoints.end();
    }

    std::map<unsigned, Debugger::Watchpoint>::iterator watchpointsBegin() {
        return _watchpoints.begin();
    }

    std::map<unsigned, Debugger::Watchpoint>::iterator watchpointsEnd() {
        return _watchpoints.end();
    }

private:
    unsigned _breakpoints_counter;
    unsigned _watchpoints_counter;
    std::map<unsigned, Breakpoint> _breakpoints;
    std::map<unsigned, Watchpoint> _watchpoints;
};

namespace debugger {

extern Debugger debugger;
extern char const *LabelName[Debugger::Label::LabelCount];
extern fmt::text_style VerbosityStyle[5];

template <Debugger::Verbosity verb>
static void vlog(Debugger::Label label, const char* format, fmt::format_args args) {
    if (debugger.verbosity[label] >= verb) {
        fmt::print(fmt::fg(debugger.color[label]), "{:>7} | ", LabelName[label]);
        fmt::vprint(::stdout, VerbosityStyle[verb], format, args);
        fmt::print("\n");
    }
}

template <typename... Args>
static void debug(Debugger::Label label, const char* format, const Args & ... args) {
    vlog<Debugger::Verbosity::Debug>(label, format, fmt::make_format_args(args...));
}

template <typename... Args>
static void info(Debugger::Label label, const char* format, const Args & ... args) {
    vlog<Debugger::Verbosity::Info>(label, format, fmt::make_format_args(args...));
}

template <typename... Args>
static void warn(Debugger::Label label, const char* format, const Args & ... args) {
    vlog<Debugger::Verbosity::Warn>(label, format, fmt::make_format_args(args...));
}

template <typename... Args>
static void error(Debugger::Label label, const char* format, const Args & ... args) {
    vlog<Debugger::Verbosity::Error>(label, format, fmt::make_format_args(args...));
}

/* Called for undefined behaviour, can be configured to hard fail the
 * emulation. */
static void undefined(std::string reason) {
}

}; /* namespace debugger */

#endif /* _DEBUGGER_H_INCLUDED_ */
