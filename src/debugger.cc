
#include <iomanip>
#include <iostream>

#include <psx/debugger.h>

/** @brief Global debugger. */
Debugger debugger::debugger;

char const *debugger::LabelName[Debugger::Label::LabelCount] = {
    "cpu", "cop0", "cop2", "mem", "joy", "int", "timer", "dma", "cdrom", "gpu"
};

fmt::text_style debugger::VerbosityStyle[] = {
    fmt::fg(fmt::color::black),
    fmt::emphasis::italic | fmt::emphasis::bold | fmt::fg(fmt::color::orange_red),
    fmt::emphasis::italic | fmt::emphasis::bold | fmt::fg(fmt::color::yellow),
    fmt::fg(fmt::color::floral_white),
    fmt::fg(fmt::color::dim_gray),
};

/* Class */

Debugger::Debugger()
    : cpu_trace(0x10000)
{
    for (int label = 0; label < Debugger::LabelCount; label++) {
        verbosity[label] = Debugger::Info;
    }

    color[Debugger::CPU] = fmt::color::cadet_blue;
    color[Debugger::COP0] = fmt::color::aquamarine;
    color[Debugger::COP2] = fmt::color::dark_orange;
    color[Debugger::MC] = fmt::color::blue_violet;
    color[Debugger::JC] = fmt::color::green_yellow;
    color[Debugger::IC] = fmt::color::chartreuse;
    color[Debugger::Timer] = fmt::color::midnight_blue;
    color[Debugger::DMA] = fmt::color::medium_sea_green;
    color[Debugger::CDROM] = fmt::color::medium_orchid;
    color[Debugger::GPU] = fmt::color::deep_pink;

#if 0
    color[Debugger::MI] = fmt::color::golden_rod;
    color[Debugger::VI] = fmt::color::medium_slate_blue;
    color[Debugger::AI] = fmt::color::coral;
    color[Debugger::PI] = fmt::color::lemon_chiffon;
    color[Debugger::RI] = fmt::color::wheat;
    color[Debugger::SI] = fmt::color::turquoise;
    color[Debugger::PIF] = fmt::color::tomato;
    color[Debugger::Cart] = fmt::color::indian_red;
#endif
}

Debugger::~Debugger() {
}

/* Breakpoints */

unsigned Debugger::set_breakpoint(uint64_t addr) {
    unsigned id = _breakpoints_counter++;
    _breakpoints[id] = Breakpoint(id, addr);
    return id;
}

void Debugger::unset_breakpoint(unsigned id) {
    _breakpoints.erase(id);
}

bool Debugger::check_breakpoint(uint64_t addr, unsigned *id) {
    for (auto bp: _breakpoints) {
        if (bp.second.addr == addr && bp.second.enabled) {
            if (id != NULL) *id = bp.second.id;
            return true;
        }
    }
    return false;
}

unsigned Debugger::set_watchpoint(uint64_t start_addr, uint64_t end_addr) {
    unsigned id = _watchpoints_counter++;
    _watchpoints[id] = Watchpoint(id, start_addr, end_addr);
    return id;
}

void Debugger::unset_watchpoint(unsigned id) {
    _watchpoints.erase(id);
}

bool Debugger::check_watchpoint(uint64_t start_addr, uint64_t end_addr,
                                unsigned *id) {
    for (auto bp: _watchpoints) {
        if (bp.second.end_addr >= start_addr &&
            bp.second.start_addr <= end_addr &&
            bp.second.enabled) {
            if (id != NULL) *id = bp.second.id;
            return true;
        }
    }
    return false;
}
