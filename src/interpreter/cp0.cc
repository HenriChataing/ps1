
#include <iomanip>
#include <iostream>
#include <cstring>

#include <psx/psx.h>
#include <psx/memory.h>
#include <psx/debugger.h>
#include <assembly/disassembler.h>
#include <assembly/opcodes.h>
#include <assembly/registers.h>
#include <interpreter.h>

using namespace psx;

namespace interpreter::cpu {

/** @brief Interpret a MFC0 instruction. */
void eval_MFC0(uint32_t instr) {
    using namespace assembly::cpu;
    uint32_t rt = assembly::getRt(instr);
    uint32_t rd = assembly::getRd(instr);
    uint32_t val;

    switch (rd) {
    case DCIC:      val = state.cp0.dcic; break;
    case BDA:       val = state.cp0.bda; break;
    case BDAM:      val = state.cp0.bdam; break;
    case BPC:       val = state.cp0.bpc; break;
    case BPCM:      val = state.cp0.bpcm; break;
    case JUMPDEST:  val = state.cp0.jumpdest; break;
    case BadVAddr:  val = state.cp0.badvaddr; break;
    case SR:        val = state.cp0.sr; break;
    case Cause:     val = state.cp0.cause; break;
    case EPC:       val = state.cp0.epc; break;
    case PrId:
        val = state.cp0.prid;
        psx::halt("MFC0 prid");
        break;
    default:
        val = 0;
        std::string reason = "MFC0 ";
        psx::halt(reason + assembly::cpu::Cop0RegisterNames[rd]);
        break;
    }

    debugger::info(Debugger::COP0, "{} -> {:08x}",
        assembly::cpu::Cop0RegisterNames[rd], val);
    state.cpu.gpr[rt] = val;
}

/** @brief Interpret a MTC0 instruction. */
void eval_MTC0(uint32_t instr) {
    using namespace assembly::cpu;
    uint32_t rt = assembly::getRt(instr);
    uint32_t rd = assembly::getRd(instr);
    uint32_t val = state.cpu.gpr[rt];

    debugger::info(Debugger::COP0, "{} <- {:08x}",
        assembly::cpu::Cop0RegisterNames[rd], val);

    switch (rd) {
    case DCIC:      state.cp0.dcic = val; break;
    case BDA:       state.cp0.bda = val; break;
    case BDAM:      state.cp0.bdam = val; break;
    case BPC:       state.cp0.bpc = val; break;
    case BPCM:      state.cp0.bpcm = val; break;
    case JUMPDEST:  state.cp0.jumpdest = val; break;
    case BadVAddr:  state.cp0.badvaddr = val; break;
    case EPC:       state.cp0.epc = val; break;

    case SR:
        // TODO check config bits
        state.cp0.sr = val;
        check_interrupt();
        break;

    case Cause:
        state.cp0.cause =
            (state.cp0.cause & ~CAUSE_IP_MASK) |
            (val             &  CAUSE_IP_MASK);
        // Interrupts bit 0 and 1 can be used to raise
        // software interrupts.
        // TODO mask unimplemented bits (only NMI, IRQ, SWI0, SWI1
        // are actually writable).
        check_interrupt();
        break;

    case PrId:
        state.cp0.prid = val;
        psx::halt("MTC0 prid");
        break;
    default:
        val = 0;
        std::string reason = "MTC0 ";
        psx::halt(reason + assembly::cpu::Cop0RegisterNames[rd]);
        break;
    }
}

/** @brief Interpret a CFC0 instruction. */
void eval_CFC0(uint32_t instr) {
    (void)instr;
    psx::halt("CFC0");
}

/** @brief Interpret a CTC0 instruction. */
void eval_CTC0(uint32_t instr) {
    (void)instr;
    psx::halt("CTC0");
}

/** @brief Interpret the RFE instruction. */
void eval_RFE(uint32_t instr) {
    uint32_t ku_ie = state.cp0.sr & UINT32_C(0x3f);
    state.cp0.sr &= ~UINT32_C(0xf);
    state.cp0.sr |= ku_ie >> 2;
    // Clearing the exception flag may have unmasked a
    // pending interrupt.
    check_interrupt();
}

void eval_COP0(uint32_t instr)
{
    switch (assembly::getRs(instr)) {
        case assembly::MFCz:          eval_MFC0(instr); break;
        case assembly::MTCz:          eval_MTC0(instr); break;
        case assembly::CFCz:          eval_CFC0(instr); break;
        case assembly::CTCz:          eval_CTC0(instr); break;
        case 0x10u:
            switch (assembly::getFunct(instr)) {
                case assembly::RFE:   eval_RFE(instr); break;
                default:
                    psx::halt("COP0 unsupported COFUN instruction");
                    break;
            }
            break;
        default:
            psx::halt("COP0 unsupported instruction");
            break;
    }
}

}; /* namespace interpreter::cpu */
