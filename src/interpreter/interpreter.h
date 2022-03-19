
#ifndef _INTERPRETER_H_INCLUDED_
#define _INTERPRETER_H_INCLUDED_

#include <psx/psx.h>
#include <lib/types.h>

namespace interpreter::cpu {

/** Fetch and execute exactly one instruction at the current
 * program counter address. */
void eval(void);

void eval_Reserved(uint32_t instr);

void eval_ADD(uint32_t instr);
void eval_ADDU(uint32_t instr);
void eval_AND(uint32_t instr);
void eval_BREAK(uint32_t instr);
void eval_DADD(uint32_t instr);
void eval_DADDU(uint32_t instr);
void eval_DDIV(uint32_t instr);
void eval_DDIVU(uint32_t instr);
void eval_DIV(uint32_t instr);
void eval_DIVU(uint32_t instr);
void eval_DMULT(uint32_t instr);
void eval_DMULTU(uint32_t instr);
void eval_DSLL(uint32_t instr);
void eval_DSLL32(uint32_t instr);
void eval_DSLLV(uint32_t instr);
void eval_DSRA(uint32_t instr);
void eval_DSRA32(uint32_t instr);
void eval_DSRAV(uint32_t instr);
void eval_DSRL(uint32_t instr);
void eval_DSRL32(uint32_t instr);
void eval_DSRLV(uint32_t instr);
void eval_DSUB(uint32_t instr);
void eval_DSUBU(uint32_t instr);
void eval_JALR(uint32_t instr);
void eval_JR(uint32_t instr);
void eval_MFHI(uint32_t instr);
void eval_MFLO(uint32_t instr);
void eval_MOVN(uint32_t instr);
void eval_MOVZ(uint32_t instr);
void eval_MTHI(uint32_t instr);
void eval_MTLO(uint32_t instr);
void eval_MULT(uint32_t instr);
void eval_MULTU(uint32_t instr);
void eval_NOR(uint32_t instr);
void eval_OR(uint32_t instr);
void eval_SLL(uint32_t instr);
void eval_SLLV(uint32_t instr);
void eval_SLT(uint32_t instr);
void eval_SLTU(uint32_t instr);
void eval_SRA(uint32_t instr);
void eval_SRAV(uint32_t instr);
void eval_SRL(uint32_t instr);
void eval_SRLV(uint32_t instr);
void eval_SUB(uint32_t instr);
void eval_SUBU(uint32_t instr);
void eval_SYNC(uint32_t instr);
void eval_SYSCALL(uint32_t instr);
void eval_TEQ(uint32_t instr);
void eval_TGE(uint32_t instr);
void eval_TGEU(uint32_t instr);
void eval_TLT(uint32_t instr);
void eval_TLTU(uint32_t instr);
void eval_TNE(uint32_t instr);
void eval_XOR(uint32_t instr);
void eval_BGEZ(uint32_t instr);
void eval_BGEZL(uint32_t instr);
void eval_BLTZ(uint32_t instr);
void eval_BLTZL(uint32_t instr);
void eval_BGEZAL(uint32_t instr);
void eval_BGEZALL(uint32_t instr);
void eval_BLTZAL(uint32_t instr);
void eval_BLTZALL(uint32_t instr);
void eval_TEQI(uint32_t instr);
void eval_TGEI(uint32_t instr);
void eval_TGEIU(uint32_t instr);
void eval_TLTI(uint32_t instr);
void eval_TLTIU(uint32_t instr);
void eval_TNEI(uint32_t instr);
void eval_ADDI(uint32_t instr);
void eval_ADDIU(uint32_t instr);
void eval_ANDI(uint32_t instr);
void eval_BEQ(uint32_t instr);
void eval_BEQL(uint32_t instr);
void eval_BGTZ(uint32_t instr);
void eval_BGTZL(uint32_t instr);
void eval_BLEZ(uint32_t instr);
void eval_BLEZL(uint32_t instr);
void eval_BNE(uint32_t instr);
void eval_BNEL(uint32_t instr);
void eval_CACHE(uint32_t instr);
void eval_COP2(uint32_t instr);
void eval_COP3(uint32_t instr);
void eval_DADDI(uint32_t instr);
void eval_DADDIU(uint32_t instr);
void eval_J(uint32_t instr);
void eval_JAL(uint32_t instr);
void eval_LB(uint32_t instr);
void eval_LBU(uint32_t instr);
void eval_LD(uint32_t instr);
void eval_LDC1(uint32_t instr);
void eval_LDC2(uint32_t instr);
void eval_LDL(uint32_t instr);
void eval_LDR(uint32_t instr);
void eval_LH(uint32_t instr);
void eval_LHU(uint32_t instr);
void eval_LL(uint32_t instr);
void eval_LLD(uint32_t instr);
void eval_LUI(uint32_t instr);
void eval_LW(uint32_t instr);
void eval_LWC1(uint32_t instr);
void eval_LWC2(uint32_t instr);
void eval_LWC3(uint32_t instr);
void eval_LWL(uint32_t instr);
void eval_LWR(uint32_t instr);
void eval_LWU(uint32_t instr);
void eval_ORI(uint32_t instr);
void eval_SB(uint32_t instr);
void eval_SC(uint32_t instr);
void eval_SCD(uint32_t instr);
void eval_SD(uint32_t instr);
void eval_SDC1(uint32_t instr);
void eval_SDC2(uint32_t instr);
void eval_SDL(uint32_t instr);
void eval_SDR(uint32_t instr);
void eval_SH(uint32_t instr);
void eval_SLTI(uint32_t instr);
void eval_SLTIU(uint32_t instr);
void eval_SW(uint32_t instr);
void eval_SWC1(uint32_t instr);
void eval_SWC2(uint32_t instr);
void eval_SWC3(uint32_t instr);
void eval_SWL(uint32_t instr);
void eval_SWR(uint32_t instr);
void eval_XORI(uint32_t instr);
void eval_SPECIAL(uint32_t instr);
void eval_REGIMM(uint32_t instr);
void eval_Instr(uint32_t instr);

void eval_MFC0(uint32_t instr);
void eval_MTC0(uint32_t instr);
void eval_DMTC0(uint32_t instr);
void eval_CFC0(uint32_t instr);
void eval_CTC0(uint32_t instr);
void eval_RFE(uint32_t instr);
void eval_COP0(uint32_t instr);

void eval_MFC2(uint32_t instr);
void eval_MTC2(uint32_t instr);
void eval_CFC2(uint32_t instr);
void eval_CTC2(uint32_t instr);
void eval_COP2(uint32_t instr);

/** Helper for branch instructions: update the state to branch to \p btrue
 * or \p bfalse depending on the tested condition \p cond. */
static inline void branch(bool cond, uint32_t btrue, uint32_t bfalse) {
    psx::state.cpu_state = psx::Delay;
    psx::state.jump_address = cond ? btrue : bfalse;
}

/** Helper for branch likely instructions: update the state to branch to
 * \p btrue or \p bfalse depending on the tested condition \p cond. */
static inline void branch_likely(bool cond, uint32_t btrue, uint32_t bfalse) {
    psx::state.cpu_state = cond ? psx::Delay : psx::Jump;
    psx::state.jump_address = cond ? btrue : bfalse;
}

}; /* namespace interpreter::cpu */

#endif /* _INTERPRETER_H_INCLUDED_ */
