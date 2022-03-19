
#include <iomanip>
#include <iostream>

#include <assembly/disassembler.h>
#include <psx/psx.h>
#include <psx/debugger.h>
#include <interpreter.h>

using namespace psx;

namespace interpreter::cpu {

/**
 * @brief Preprocessor template for I-type instructions.
 *
 * The registers and immediate value are automatically extracted from the
 * instruction and added as rs, rt, imm in a new scope.
 *
 * @param instr             Original instruction
 * @param extend            Extension method (sign or zero extend)
 */
#define IType(instr, extend) \
    uint32_t rs = assembly::getRs(instr); \
    uint32_t rt = assembly::getRt(instr); \
    uint32_t imm = extend<uint32_t, uint16_t>(assembly::getImmediate(instr)); \
    (void)rs; (void)rt; (void)imm;

/**
 * @brief Preprocessor template for R-type instructions.
 *
 * The registers are automatically extracted from the instruction and added
 * as rd, rs, rt, shamnt in a new scope.
 *
 * @param instr             Original instruction
 */
#define RType(instr) \
    uint32_t rd = assembly::getRd(instr); \
    uint32_t rs = assembly::getRs(instr); \
    uint32_t rt = assembly::getRt(instr); \
    uint32_t shamnt = assembly::getShamnt(instr); \
    (void)rd; (void)rs; (void)rt; (void)shamnt;

/**
 * Check whether a virtual memory address is correctly aligned
 * for a memory access, raise AddressError otherwise.
 */
#define checkAddressAlignment(vAddr, bytes, instr, load)                       \
    if (((vAddr) & ((bytes) - 1)) != 0) {                                      \
        take_exception(AddressError, (vAddr), (instr), (load));                 \
        return;                                                                \
    }

/**
 * Check whether Cop1 is currently enabled in SR,
 * raise CoprocessorUnusable otherwise.
 */
#define checkCop1Usable()                                                      \
    if (!state.cp0reg.CU1()) {                                                 \
        take_exception(CoprocessorUnusable, 0, instr, false, 1u);               \
        return;                                                                \
    }

/**
 * Take the exception \p exn and return from the current function
 * if it is not None.
 */
#define checkException(exn, vAddr, instr, load, ce)                            \
({                                                                             \
    enum cpu_exception __exn = (exn);                                                   \
    if (__exn != cpu_exception::None) {                                            \
        take_exception(__exn, (vAddr), (instr), (load), (ce));                  \
        return;                                                                \
    }                                                                          \
})


void eval_Reserved(uint32_t instr) {
    psx::halt("CPU reserved instruction");
}

/* SPECIAL opcodes */

void eval_ADD(uint32_t instr) {
    RType(instr);
    int32_t a = (int32_t)(uint32_t)state.cpu.gpr[rs];
    int32_t b = (int32_t)(uint32_t)state.cpu.gpr[rt];
    int32_t res;

    if (__builtin_add_overflow(a, b, &res)) {
        psx::halt("ADD IntegerOverflow");
        take_exception(IntegerOverflow, 0, 0, 0);
    } else {
        state.cpu.gpr[rd] = (uint32_t)res;
    }
}

void eval_ADDU(uint32_t instr) {
    RType(instr);
    uint32_t a = (uint32_t)state.cpu.gpr[rs];
    uint32_t b = (uint32_t)state.cpu.gpr[rt];
    uint32_t res = a + b;

    state.cpu.gpr[rd] = res;
}

void eval_AND(uint32_t instr) {
    RType(instr);
    state.cpu.gpr[rd] = state.cpu.gpr[rs] & state.cpu.gpr[rt];
}

void eval_BREAK(uint32_t instr) {
    RType(instr);
    psx::halt("BREAK");
}

void eval_DIV(uint32_t instr) {
    RType(instr);
    // Must use 64bit integers here to prevent signed overflow.
    int64_t num   = (int32_t)state.cpu.gpr[rs];
    int64_t denum = (int32_t)state.cpu.gpr[rt];
    if (denum != 0) {
        state.cpu.mult_lo = (uint32_t)(uint64_t)(num / denum);
        state.cpu.mult_hi = (uint32_t)(uint64_t)(num % denum);
    } else {
        debugger::undefined("Divide by 0 (DIV)");
        // Undefined behaviour here according to the reference
        // manual. The machine behaviour is as implemented.
        state.cpu.mult_lo = num < 0 ? 1 : UINT32_C(-1);
        state.cpu.mult_hi = (uint32_t)(uint64_t)num;
    }
}

void eval_DIVU(uint32_t instr) {
    RType(instr);
    uint32_t num = state.cpu.gpr[rs];
    uint32_t denum = state.cpu.gpr[rt];
    if (denum != 0) {
        state.cpu.mult_lo = (uint32_t)(num / denum);
        state.cpu.mult_hi = (uint32_t)(num % denum);
    } else {
        debugger::undefined("Divide by 0 (DIVU)");
        // Undefined behaviour here according to the reference
        // manual. The machine behaviour is as implemented.
        state.cpu.mult_lo = UINT32_C(-1);
        state.cpu.mult_hi = num;
    }
}

void eval_JALR(uint32_t instr) {
    RType(instr);
    uint32_t tg = state.cpu.gpr[rs];
    state.cpu.gpr[rd] = state.cpu.pc + 8;
    state.cpu_state = cpu_state::Delay;
    state.jump_address = tg;
}

void eval_JR(uint32_t instr) {
    RType(instr);
    uint32_t tg = state.cpu.gpr[rs];
    state.cpu_state = cpu_state::Delay;
    state.jump_address = tg;
}

void eval_MFHI(uint32_t instr) {
    RType(instr);
    // undefined if an instruction that follows modify
    // the special registers LO / HI
    state.cpu.gpr[rd] = state.cpu.mult_hi;
}

void eval_MFLO(uint32_t instr) {
    RType(instr);
    // undefined if an instruction that follows modify
    // the special registers LO / HI
    state.cpu.gpr[rd] = state.cpu.mult_lo;
}

void eval_MOVN(uint32_t instr) {
    RType(instr);
    psx::halt("MOVN");
}

void eval_MOVZ(uint32_t instr) {
    RType(instr);
    psx::halt("MOVZ");
}

void eval_MTHI(uint32_t instr) {
    RType(instr);
    state.cpu.mult_hi = state.cpu.gpr[rs];
}

void eval_MTLO(uint32_t instr) {
    RType(instr);
    state.cpu.mult_lo = state.cpu.gpr[rs];
}

void eval_MULT(uint32_t instr) {
    RType(instr);
    int32_t a = (int32_t)(uint32_t)state.cpu.gpr[rs];
    int32_t b = (int32_t)(uint32_t)state.cpu.gpr[rt];
    int64_t m = (int64_t)a * (int64_t)b;
    state.cpu.mult_lo = (uint32_t)((uint64_t)m >>  0);
    state.cpu.mult_hi = (uint32_t)((uint64_t)m >> 32);
}

void eval_MULTU(uint32_t instr) {
    RType(instr);
    uint32_t a = (uint32_t)state.cpu.gpr[rs];
    uint32_t b = (uint32_t)state.cpu.gpr[rt];
    uint64_t m = (uint64_t)a * (uint64_t)b;
    state.cpu.mult_lo = m;
    state.cpu.mult_hi = m >> 32;
}

void eval_NOR(uint32_t instr) {
    RType(instr);
    state.cpu.gpr[rd] = ~(state.cpu.gpr[rs] | state.cpu.gpr[rt]);
}

void eval_OR(uint32_t instr) {
    RType(instr);
    state.cpu.gpr[rd] = state.cpu.gpr[rs] | state.cpu.gpr[rt];
}

void eval_SLL(uint32_t instr) {
    RType(instr);
    state.cpu.gpr[rd] = state.cpu.gpr[rt] << shamnt;
}

void eval_SLLV(uint32_t instr) {
    RType(instr);
    shamnt = state.cpu.gpr[rs] & UINT32_C(0x1f);
    state.cpu.gpr[rd] = state.cpu.gpr[rt] << shamnt;
}

void eval_SLT(uint32_t instr) {
    RType(instr);
    state.cpu.gpr[rd] = (int32_t)state.cpu.gpr[rs] < (int32_t)state.cpu.gpr[rt];
}

void eval_SLTU(uint32_t instr) {
    RType(instr);
    state.cpu.gpr[rd] = state.cpu.gpr[rs] < state.cpu.gpr[rt];
}

void eval_SRA(uint32_t instr) {
    RType(instr);
    bool sign = (state.cpu.gpr[rt] & (UINT32_C(1) << 31)) != 0;
    // Right shift is logical for unsigned c types,
    // we need to add the type manually.
    state.cpu.gpr[rd] = state.cpu.gpr[rt] >> shamnt;
    if (sign) {
        uint32_t mask = (UINT32_C(1) << shamnt) - 1u;
        state.cpu.gpr[rd] |= mask << (32 - shamnt);
    }
}

void eval_SRAV(uint32_t instr) {
    RType(instr);
    bool sign = (state.cpu.gpr[rt] & (UINT32_C(1) << 31)) != 0;
    shamnt = state.cpu.gpr[rs] & UINT32_C(0x1f);
    // Right shift is logical for unsigned c types,
    // we need to add the type manually.
    state.cpu.gpr[rd] = state.cpu.gpr[rt] >> shamnt;
    if (sign) {
        uint32_t mask = (UINT32_C(1) << shamnt) - 1u;
        state.cpu.gpr[rd] |= mask << (32 - shamnt);
    }
}

void eval_SRL(uint32_t instr) {
    RType(instr);
    state.cpu.gpr[rd] = state.cpu.gpr[rt] >> shamnt;
}

void eval_SRLV(uint32_t instr) {
    RType(instr);
    shamnt = state.cpu.gpr[rs] & UINT32_C(0x1f);
    state.cpu.gpr[rd] = state.cpu.gpr[rt] >> shamnt;
}

void eval_SUB(uint32_t instr) {
    RType(instr);
    int32_t res;
    int32_t a = (int32_t)(uint32_t)state.cpu.gpr[rs];
    int32_t b = (int32_t)(uint32_t)state.cpu.gpr[rt];
    if (__builtin_sub_overflow(a, b, &res)) {
        psx::halt("SUB IntegerOverflow");
    }
    state.cpu.gpr[rd] = (uint32_t)res;
}

void eval_SUBU(uint32_t instr) {
    RType(instr);
    state.cpu.gpr[rd] = state.cpu.gpr[rs] - state.cpu.gpr[rt];
}

void eval_SYNC(uint32_t instr) {
    (void)instr;
}

void eval_SYSCALL(uint32_t instr) {
    RType(instr);
    take_exception(SystemCall, 0, false, false, 0);
}

void eval_TEQ(uint32_t instr) {
    RType(instr);
    psx::halt("TEQ");
}

void eval_TGE(uint32_t instr) {
    RType(instr);
    psx::halt("TGE");
}

void eval_TGEU(uint32_t instr) {
    RType(instr);
    psx::halt("TGEU");
}

void eval_TLT(uint32_t instr) {
    RType(instr);
    psx::halt("TLT");
}

void eval_TLTU(uint32_t instr) {
    RType(instr);
    psx::halt("TLTU");
}

void eval_TNE(uint32_t instr) {
    RType(instr);
    psx::halt("TNE");
}

void eval_XOR(uint32_t instr) {
    RType(instr);
    state.cpu.gpr[rd] = state.cpu.gpr[rs] ^ state.cpu.gpr[rt];
}

/* REGIMM opcodes */

void eval_BGEZ(uint32_t instr) {
    IType(instr, sign_extend);
    branch((int32_t)state.cpu.gpr[rs] >= 0,
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_BGEZL(uint32_t instr) {
    IType(instr, sign_extend);
    branch_likely((int32_t)state.cpu.gpr[rs] >= 0,
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_BLTZ(uint32_t instr) {
    IType(instr, sign_extend);
    branch((int32_t)state.cpu.gpr[rs] < 0,
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_BLTZL(uint32_t instr) {
    IType(instr, sign_extend);
    branch_likely((int32_t)state.cpu.gpr[rs] < 0,
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_BGEZAL(uint32_t instr) {
    IType(instr, sign_extend);
    int32_t r = state.cpu.gpr[rs];
    state.cpu.gpr[31] = state.cpu.pc + 8;
    branch(r >= 0,
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_BGEZALL(uint32_t instr) {
    IType(instr, sign_extend);
    int32_t r = state.cpu.gpr[rs];
    state.cpu.gpr[31] = state.cpu.pc + 8;
    branch_likely(r >= 0,
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_BLTZAL(uint32_t instr) {
    IType(instr, sign_extend);
    int32_t r = state.cpu.gpr[rs];
    state.cpu.gpr[31] = state.cpu.pc + 8;
    branch(r < 0,
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_BLTZALL(uint32_t instr) {
    IType(instr, sign_extend);
    int32_t r = state.cpu.gpr[rs];
    state.cpu.gpr[31] = state.cpu.pc + 8;
    branch_likely(r < 0,
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_TEQI(uint32_t instr) {
    IType(instr, sign_extend);
    psx::halt("TEQI");
}

void eval_TGEI(uint32_t instr) {
    IType(instr, sign_extend);
    psx::halt("TGEI");
}

void eval_TGEIU(uint32_t instr) {
    IType(instr, sign_extend);
    psx::halt("TGEIU");
}

void eval_TLTI(uint32_t instr) {
    IType(instr, sign_extend);
    psx::halt("TLTI");
}

void eval_TLTIU(uint32_t instr) {
    IType(instr, sign_extend);
    psx::halt("TLTIU");
}

void eval_TNEI(uint32_t instr) {
    IType(instr, sign_extend);
    psx::halt("TNEI");
}

/* Other opcodes */

void eval_ADDI(uint32_t instr) {
    IType(instr, sign_extend);
    int32_t a = (int32_t)(uint32_t)state.cpu.gpr[rs];
    int32_t b = (int32_t)(uint32_t)imm;
    int32_t res;

    if (__builtin_add_overflow(a, b, &res)) {
        psx::halt("ADDI IntegerOverflow");
        take_exception(IntegerOverflow, 0, 0, 0);
    } else {
        state.cpu.gpr[rt] = (uint32_t)res;
    }
}

void eval_ADDIU(uint32_t instr) {
    IType(instr, sign_extend);
    uint32_t a = (uint32_t)state.cpu.gpr[rs];
    uint32_t b = (uint32_t)imm;
    uint32_t res = a + b;

    state.cpu.gpr[rt] = res;
}

void eval_ANDI(uint32_t instr) {
    IType(instr, zero_extend);
    state.cpu.gpr[rt] = state.cpu.gpr[rs] & imm;
}

void eval_BEQ(uint32_t instr) {
    IType(instr, sign_extend);
    branch(state.cpu.gpr[rt] == state.cpu.gpr[rs],
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_BEQL(uint32_t instr) {
    IType(instr, sign_extend);
    branch_likely(state.cpu.gpr[rt] == state.cpu.gpr[rs],
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_BGTZ(uint32_t instr) {
    IType(instr, sign_extend);
    branch((int32_t)state.cpu.gpr[rs] > 0,
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_BGTZL(uint32_t instr) {
    IType(instr, sign_extend);
    branch_likely((int32_t)state.cpu.gpr[rs] > 0,
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_BLEZ(uint32_t instr) {
    IType(instr, sign_extend);
    branch((int32_t)state.cpu.gpr[rs] <= 0,
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_BLEZL(uint32_t instr) {
    IType(instr, sign_extend);
    branch_likely((int32_t)state.cpu.gpr[rs] <= 0,
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_BNE(uint32_t instr) {
    IType(instr, sign_extend);
    branch(state.cpu.gpr[rt] != state.cpu.gpr[rs],
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_BNEL(uint32_t instr) {
    IType(instr, sign_extend);
    branch_likely(state.cpu.gpr[rt] != state.cpu.gpr[rs],
        state.cpu.pc + 4 + (int32_t)(imm << 2),
        state.cpu.pc + 8);
}

void eval_CACHE(uint32_t instr) {
    (void)instr;
}

void eval_COP1(uint32_t instr) {
    take_exception(CoprocessorUnusable, 0, false, false, 1);
}

void eval_COP3(uint32_t instr) {
    take_exception(CoprocessorUnusable, 0, false, false, 3);
}

void eval_J(uint32_t instr) {
    uint32_t tg = assembly::getTarget(instr);
    tg = (state.cpu.pc & UINT32_C(0xf0000000)) | (tg << 2);
    state.cpu_state = cpu_state::Delay;
    state.jump_address = tg;
}

void eval_JAL(uint32_t instr) {
    uint32_t tg = assembly::getTarget(instr);
    tg = (state.cpu.pc & UINT32_C(0xf0000000)) | (tg << 2);
    state.cpu.gpr[31] = state.cpu.pc + 8;
    state.cpu_state = cpu_state::Delay;
    state.jump_address = tg;
}

void eval_LB(uint32_t instr) {
    IType(instr, sign_extend);

    uint32_t vAddr = state.cpu.gpr[rs] + imm;
    uint32_t pAddr;
    uint8_t val;

    checkException(
        translate_address(vAddr, &pAddr, false),
        vAddr, false, true, 0);
    checkException(
        state.bus->load_u8(pAddr, &val) ? None : BusError,
        vAddr, false, true, 0);

    state.cpu.gpr[rt] = sign_extend<uint32_t, uint8_t>(val);
}

void eval_LBU(uint32_t instr) {
    IType(instr, sign_extend);

    uint32_t vAddr = state.cpu.gpr[rs] + imm;
    uint32_t pAddr;
    uint8_t val;

    checkException(
        translate_address(vAddr, &pAddr, false),
        vAddr, false, true, 0);
    checkException(
        state.bus->load_u8(pAddr, &val) ? None : BusError,
        vAddr, false, true, 0);

    state.cpu.gpr[rt] = zero_extend<uint32_t, uint8_t>(val);
}

void eval_LH(uint32_t instr) {
    IType(instr, sign_extend);

    uint32_t vAddr = state.cpu.gpr[rs] + imm;
    uint32_t pAddr;
    uint16_t val;

    checkAddressAlignment(vAddr, 2, false, true);
    checkException(
        translate_address(vAddr, &pAddr, false),
        vAddr, false, true, 0);
    checkException(
        state.bus->load_u16(pAddr, &val) ? None : BusError,
        vAddr, false, true, 0);

    state.cpu.gpr[rt] = sign_extend<uint32_t, uint16_t>(val);
}

void eval_LHU(uint32_t instr) {
    IType(instr, sign_extend);

    uint32_t vAddr = state.cpu.gpr[rs] + imm;
    uint32_t pAddr;
    uint16_t val;

    checkAddressAlignment(vAddr, 2, false, true);
    checkException(
        translate_address(vAddr, &pAddr, false),
        vAddr, false, true, 0);
    checkException(
        state.bus->load_u16(pAddr, &val) ? None : BusError,
        vAddr, false, true, 0);

    state.cpu.gpr[rt] = zero_extend<uint32_t, uint16_t>(val);
}

void eval_LL(uint32_t instr) {
    IType(instr, sign_extend);
    psx::halt("LL");
}

void eval_LLD(uint32_t instr) {
    IType(instr, sign_extend);
    psx::halt("LLD");
}

void eval_LUI(uint32_t instr) {
    IType(instr, sign_extend);
    state.cpu.gpr[rt] = imm << 16;
}

void eval_LW(uint32_t instr) {
    IType(instr, sign_extend);

    uint32_t vAddr = state.cpu.gpr[rs] + imm;
    uint32_t pAddr;
    uint32_t val;

    checkAddressAlignment(vAddr, 4, false, true);
    checkException(
        translate_address(vAddr, &pAddr, false),
        vAddr, false, true, 0);
    checkException(
        state.bus->load_u32(pAddr, &val) ? None : BusError,
        vAddr, false, true, 0);

    state.cpu.gpr[rt] = val;
}

void eval_LWC1(uint32_t instr) {
    take_exception(CoprocessorUnusable, 0, false, true, 1);
    psx::halt("LWC1");
}

void eval_LWC2(uint32_t instr) {
    take_exception(CoprocessorUnusable, 0, false, true, 2);
    psx::halt("LWC2");
}

void eval_LWC3(uint32_t instr) {
    take_exception(CoprocessorUnusable, 0, false, true, 3);
    psx::halt("LWC3");
}

void eval_LWL(uint32_t instr) {
    IType(instr, sign_extend);

    // @todo only BigEndianMem & !ReverseEndian for now
    uint32_t vAddr = state.cpu.gpr[rs] + imm;
    uint32_t pAddr;

    // Not calling checkAddressAlignment:
    // this instruction specifically ignores the alignment
    checkException(
        translate_address(vAddr, &pAddr, true),
        vAddr, false, false, 0);

    size_t count = 4 - (pAddr % 4);
    unsigned int shift = 24;
    uint32_t mask = (UINT32_C(1) << (32 - 8 * count)) - 1u;
    uint32_t val = 0;

    for (size_t nr = 0; nr < count; nr++, shift -= 8) {
        uint8_t byte = 0;
        if (!state.bus->load_u8(pAddr + nr, &byte)) {
            take_exception(BusError, vAddr, false, false, 0);
            return;
        }
        val |= ((uint32_t)byte << shift);
    }

    val = val | (state.cpu.gpr[rt] & mask);
    state.cpu.gpr[rt] = val;
}

void eval_LWR(uint32_t instr) {
    IType(instr, sign_extend);

    // @todo only BigEndianMem & !ReverseEndian for now
    uint32_t vAddr = state.cpu.gpr[rs] + imm;
    uint32_t pAddr;

    // Not calling checkAddressAlignment:
    // this instruction specifically ignores the alignment
    checkException(
        translate_address(vAddr, &pAddr, true),
        vAddr, false, false, 0);

    size_t count = 1 + (pAddr % 4);
    unsigned int shift = 0;
    uint32_t mask = (UINT32_C(1) << (32 - 8 * count)) - 1u;
    mask <<= 8 * count;
    uint32_t val = 0;

    for (size_t nr = 0; nr < count; nr++, shift += 8) {
        uint8_t byte = 0;
        if (!state.bus->load_u8(pAddr - nr, &byte)) {
            take_exception(BusError, vAddr, false, false);
            return;
        }
        val |= ((uint32_t)byte << shift);
    }

    val = val | (state.cpu.gpr[rt] & mask);
    state.cpu.gpr[rt] = val;
}

void eval_LWU(uint32_t instr) {
    IType(instr, sign_extend);

    uint32_t vAddr = state.cpu.gpr[rs] + imm;
    uint32_t pAddr;
    uint32_t val;

    checkAddressAlignment(vAddr, 4, false, true);
    checkException(
        translate_address(vAddr, &pAddr, false),
        vAddr, false, true, 0);
    checkException(
        state.bus->load_u32(pAddr, &val) ? None : BusError,
        vAddr, false, true, 0);

    state.cpu.gpr[rt] = val;
}

void eval_ORI(uint32_t instr) {
    IType(instr, zero_extend);
    state.cpu.gpr[rt] = state.cpu.gpr[rs] | imm;
}

void eval_SB(uint32_t instr) {
    IType(instr, sign_extend);

    uint32_t vAddr = state.cpu.gpr[rs] + imm;
    uint32_t pAddr;

    checkException(
        translate_address(vAddr, &pAddr, false),
        vAddr, false, false, 0);
    checkException(
        state.bus->store_u8(pAddr, state.cpu.gpr[rt]) ? None : BusError,
        vAddr, false, false, 0);
}

void eval_SC(uint32_t instr) {
    IType(instr, sign_extend);
    psx::halt("SC");
}

void eval_SH(uint32_t instr) {
    IType(instr, sign_extend);

    uint32_t vAddr = state.cpu.gpr[rs] + imm;
    uint32_t pAddr;

    checkAddressAlignment(vAddr, 2, false, false);
    checkException(
        translate_address(vAddr, &pAddr, false),
        vAddr, false, false, 0);
    checkException(
        state.bus->store_u16(pAddr, state.cpu.gpr[rt]) ? None : BusError,
        vAddr, false, false, 0);
}

void eval_SLTI(uint32_t instr) {
    IType(instr, sign_extend);
    state.cpu.gpr[rt] = (int32_t)state.cpu.gpr[rs] < (int32_t)imm;
}

void eval_SLTIU(uint32_t instr) {
    IType(instr, sign_extend);
    state.cpu.gpr[rt] = state.cpu.gpr[rs] < imm;
}

void eval_SW(uint32_t instr) {
    IType(instr, sign_extend);

    uint32_t vAddr = state.cpu.gpr[rs] + imm;
    uint32_t pAddr;

    checkAddressAlignment(vAddr, 4, false, false);
    checkException(
        translate_address(vAddr, &pAddr, false),
        vAddr, false, false, 0);
    checkException(
        state.bus->store_u32(pAddr, state.cpu.gpr[rt]) ? None : BusError,
        vAddr, false, false, 0);
}

void eval_SWC1(uint32_t instr) {
    take_exception(CoprocessorUnusable, 0, false, false, 1);
    psx::halt("SWC1");
}

void eval_SWC2(uint32_t instr) {
    take_exception(CoprocessorUnusable, 0, false, false, 2);
    psx::halt("SWC2");
}

void eval_SWC3(uint32_t instr) {
    take_exception(CoprocessorUnusable, 0, false, false, 2);
    psx::halt("SWC3");
}

void eval_SWL(uint32_t instr) {
    IType(instr, sign_extend);
    // psx::halt("SWL instruction");
    // @todo only BigEndianMem & !ReverseEndian for now
    uint32_t vAddr = state.cpu.gpr[rs] + imm;
    uint32_t pAddr;

    // Not calling checkAddressAlignment:
    // this instruction specifically ignores the alignment
    checkException(
        translate_address(vAddr, &pAddr, false),
        vAddr, false, false, 0);

    size_t count = 4 - (pAddr % 4);
    uint32_t val = state.cpu.gpr[rt];
    unsigned int shift = 24;
    for (size_t nr = 0; nr < count; nr++, shift -= 8) {
        uint8_t byte = val >> shift;
        if (!state.bus->store_u8(pAddr + nr, byte)) {
            take_exception(BusError, vAddr, false, false, 0);
            return;
        }
    }
}

void eval_SWR(uint32_t instr) {
    IType(instr, sign_extend);
    // psx::halt("SWR instruction");
    // @todo only BigEndianMem & !ReverseEndian for now
    uint32_t vAddr = state.cpu.gpr[rs] + imm;
    uint32_t pAddr;

    // Not calling checkAddressAlignment:
    // this instruction specifically ignores the alignment
    checkException(
        translate_address(vAddr, &pAddr, false),
        vAddr, false, false, 0);

    size_t count = 1 + (pAddr % 4);
    uint32_t val = state.cpu.gpr[rt];
    unsigned int shift = 0;
    for (size_t nr = 0; nr < count; nr++, shift += 8) {
        uint8_t byte = val >> shift;
        if (!state.bus->store_u8(pAddr - nr, byte)) {
            take_exception(BusError, vAddr, false, false);
            return;
        }
    }
}

void eval_XORI(uint32_t instr) {
    IType(instr, zero_extend);
    state.cpu.gpr[rt] = state.cpu.gpr[rs] ^ imm;
}


static void (*SPECIAL_callbacks[64])(uint32_t) = {
    eval_SLL,       eval_Reserved,  eval_SRL,       eval_SRA,
    eval_SLLV,      eval_Reserved,  eval_SRLV,      eval_SRAV,
    eval_JR,        eval_JALR,      eval_MOVZ,      eval_MOVN,
    eval_SYSCALL,   eval_BREAK,     eval_Reserved,  eval_SYNC,
    eval_MFHI,      eval_MTHI,      eval_MFLO,      eval_MTLO,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_MULT,      eval_MULTU,     eval_DIV,       eval_DIVU,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_ADD,       eval_ADDU,      eval_SUB,       eval_SUBU,
    eval_AND,       eval_OR,        eval_XOR,       eval_NOR,
    eval_Reserved,  eval_Reserved,  eval_SLT,       eval_SLTU,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_TGE,       eval_TGEU,      eval_TLT,       eval_TLTU,
    eval_TEQ,       eval_Reserved,  eval_TNE,       eval_Reserved,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
};

void eval_SPECIAL(uint32_t instr) {
    SPECIAL_callbacks[assembly::getFunct(instr)](instr);
}

static void (*REGIMM_callbacks[32])(uint32_t) = {
    eval_BLTZ,      eval_BGEZ,      eval_BLTZL,     eval_BGEZL,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_TGEI,      eval_TGEIU,     eval_TLTI,      eval_TLTIU,
    eval_TEQI,      eval_Reserved,  eval_TNEI,      eval_Reserved,
    eval_BLTZAL,    eval_BGEZAL,    eval_BLTZALL,   eval_BGEZALL,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
};

void eval_REGIMM(uint32_t instr) {
    REGIMM_callbacks[assembly::getRt(instr)](instr);
}

static void (*CPU_callbacks[64])(uint32_t) = {
    eval_SPECIAL,   eval_REGIMM,    eval_J,         eval_JAL,
    eval_BEQ,       eval_BNE,       eval_BLEZ,      eval_BGTZ,
    eval_ADDI,      eval_ADDIU,     eval_SLTI,      eval_SLTIU,
    eval_ANDI,      eval_ORI,       eval_XORI,      eval_LUI,
    eval_COP0,      eval_COP1,      eval_COP2,      eval_COP3,
    eval_BEQL,      eval_BNEL,      eval_BLEZL,     eval_BGTZL,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_LB,        eval_LH,        eval_LWL,       eval_LW,
    eval_LBU,       eval_LHU,       eval_LWR,       eval_LWU,
    eval_SB,        eval_SH,        eval_SWL,       eval_SW,
    eval_Reserved,  eval_Reserved,  eval_SWR,       eval_CACHE,
    eval_LL,        eval_LWC1,      eval_LWC2,      eval_LWC3,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_SC,        eval_SWC1,      eval_SWC2,      eval_SWC3,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
};

void eval_Instr(uint32_t instr) {
    // The null instruction is 'sll r0, r0, 0', i.e. a NOP.
    // As it is one of the most used instructions (to fill in delay slots),
    // perform a quick check to spare the instruction execution.
    if (instr) CPU_callbacks[assembly::getOpcode(instr)](instr);

    // BIOS code writes to zero register to discard the value.
    // Force reset it.
    state.cpu.gpr[0] = 0;
}

void eval(void) {
    uint32_t vaddr = state.cpu.pc;
    uint32_t paddr;
    uint32_t instr;

    state.cycles++;

    checkException(
        translate_address(vaddr, &paddr, false),
        vaddr, true, true, 0);
    checkException(
        state.bus->load_u32(paddr, &instr) ? None : BusError,
        vaddr, true, true, 0);

#if ENABLE_TRACE
    debugger::debugger.cpu_trace.put(Debugger::TraceEntry(vaddr, instr));
#endif /* ENABLE_TRACE */

#if ENABLE_BREAKPOINTS
    if (debugger::debugger.check_breakpoint(vaddr))
        psx::halt("Breakpoint");
#endif /* ENABLE_BREAKPOINTS */

    eval_Instr(instr);
}

}; /* namespace interpreter::cpu */
