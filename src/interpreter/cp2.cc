
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

//  31-25  Must be 0100101b for "COP2 imm25" instructions
//  20-24  Fake GTE Command Number (00h..1Fh) (ignored by hardware)
//  19     sf - Shift Fraction in IR registers (0=No fraction, 1=12bit fraction)
//  17-18  MVMVA Multiply Matrix    (0=Rotation. 1=Light, 2=Color, 3=Reserved)
//  15-16  MVMVA Multiply Vector    (0=V0, 1=V1, 2=V2, 3=IR/long)
//  13-14  MVMVA Translation Vector (0=TR, 1=BK, 2=FC/Bugged, 3=None)
//  11-12  Always zero                        (ignored by hardware)
//  10     lm - Saturate IR1,IR2,IR3 result (0=To -8000h..+7FFFh, 1=To 0..+7FFFh)
//  6-9    Always zero                        (ignored by hardware)
//  0-5    Real GTE Command Number (00h..3Fh) (used by hardware)

static bool get_sf(uint32_t instr) {
    return (instr >> 19) & UINT32_C(1);
}

static uint32_t get_mvmva_mm(uint32_t instr) {
    return (instr >> 17) & UINT32_C(0x3);
}

static uint32_t get_mvmva_mv(uint32_t instr) {
    return (instr >> 15) & UINT32_C(0x3);
}

static uint32_t get_mvmva_tv(uint32_t instr) {
    return (instr >> 13) & UINT32_C(0x3);
}

static bool get_lm(uint32_t instr) {
    return (instr >> 10) & UINT32_C(1);
}

static uint32_t get_opc(uint32_t instr) {
    return instr & UINT32_C(0x2f);
}

/// Perspective Transformation single.
static void eval_RTPS(uint32_t instr) {
    psx::halt("RTPS unimplemented");
}

/// Normal clipping.
static void eval_NCLIP(uint32_t instr) {
    psx::halt("NCLIP unimplemented");
}

/// Outer product of 2 vectors.
static void eval_OP(uint32_t instr) {
    psx::halt("OP(sf) unimplemented");
}

/// Depth Cueing single.
static void eval_DPCS(uint32_t instr) {
    psx::halt("DPCS unimplemented");
}

/// Interpolation of a vector and far color vector.
static void eval_INTPL(uint32_t instr) {
    psx::halt("INTPL unimplemented");
}

/// Multiply vector by matrix and add vector.
static void eval_MVMVA(uint32_t instr) {
    psx::halt("MVMVA unimplemented");
}

/// Normal color depth cue single vector.
static void eval_NCDS(uint32_t instr) {
    psx::halt("NCDS unimplemented");
}

/// Color Depth Que.
static void eval_CDP(uint32_t instr) {
    psx::halt("CDP unimplemented");
}

/// Normal color depth cue triple vectors.
static void eval_NCDT(uint32_t instr) {
    psx::halt("NCDT unimplemented");
}

/// Normal Color Color single vector.
static void eval_NCCS(uint32_t instr) {
    psx::halt("NCCS unimplemented");
}

/// Color Color.
static void eval_CC(uint32_t instr) {
    psx::halt("CC unimplemented");
}

/// Normal color single.
static void eval_NCS(uint32_t instr) {
    psx::halt("NCS unimplemented");
}

/// Normal color triple.
static void eval_NCT(uint32_t instr) {
    psx::halt("NCT unimplemented");
}

/// Square of vector IR.
static void eval_SQR(uint32_t instr) {
    psx::halt("SQR(sf) unimplemented");
}

/// Depth Cue Color light.
static void eval_DCPL(uint32_t instr) {
    psx::halt("DCPL unimplemented");
}

/// Depth Cueing triple.
static void eval_DPCT(uint32_t instr) {
    psx::halt("DPCT unimplemented");
}

/// Average of three Z values.
static void eval_AVSZ3(uint32_t instr) {
    psx::halt("AVSZ3 unimplemented");
}

/// Average of four Z values.
static void eval_AVSZ4(uint32_t instr) {
    psx::halt("AVSZ4 unimplemented");
}

/// Perspective Transformation triple.
static void eval_RTPT(uint32_t instr) {
    psx::halt("RTPT unimplemented");
}

/// General purpose interpolation.
static void eval_GPF(uint32_t instr) {
    psx::halt("GPF(sf) unimplemented");
}

/// General purpose interpolation with base.
static void eval_GPL(uint32_t instr) {
    psx::halt("GPL(sf) unimplemented");
}

/// Normal Color Color triple vector.
static void eval_NCCT(uint32_t instr) {
    psx::halt("NCCT unimplemented");
}

static void (*COP2_callbacks[64])(uint32_t) = {
    eval_Reserved,  eval_RTPS,      eval_Reserved,  eval_Reserved,
    eval_Reserved,  eval_Reserved,  eval_NCLIP,     eval_Reserved,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_OP,        eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_DPCS,      eval_INTPL,     eval_MVMVA,     eval_NCDS,
    eval_CDP,       eval_Reserved,  eval_NCDT,      eval_Reserved,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_NCCS,
    eval_CC,        eval_Reserved,  eval_NCS,       eval_Reserved,
    eval_NCT,       eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_SQR,       eval_DCPL,      eval_DPCT,      eval_Reserved,
    eval_Reserved,  eval_AVSZ3,     eval_AVSZ4,     eval_Reserved,
    eval_RTPT,      eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_Reserved,  eval_Reserved,  eval_Reserved,  eval_Reserved,
    eval_Reserved,  eval_GPF,       eval_GPL,       eval_NCCT,
};

/** @brief Interpret a MFC2 instruction. */
void eval_MFC2(uint32_t instr) {
    psx::halt("MFC2");
}

/** @brief Interpret a MTC2 instruction. */
void eval_MTC2(uint32_t instr) {
    psx::halt("MTC2");
}

/** @brief Interpret a CFC2 instruction. */
void eval_CFC2(uint32_t instr) {
    uint32_t rt = assembly::getRt(instr);
    uint32_t rd = assembly::getRd(instr);
    state.cpu.gpr[rt] = state.cp2.cr[rd];
}

/** @brief Interpret a CTC2 instruction. */
void eval_CTC2(uint32_t instr) {
    uint32_t rt = assembly::getRt(instr);
    uint32_t rd = assembly::getRd(instr);
    state.cp2.cr[rd] = state.cpu.gpr[rt];
}

void eval_COP2(uint32_t instr)
{
    if (instr & (UINT32_C(1) << 25)) {
        COP2_callbacks[get_opc(instr)](instr);
    } else {
        switch (assembly::getRs(instr)) {
        case assembly::MFCz: eval_MFC2(instr); break;
        case assembly::MTCz: eval_MTC2(instr); break;
        case assembly::CFCz: eval_CFC2(instr); break;
        case assembly::CTCz: eval_CTC2(instr); break;
        default:             eval_Reserved(instr); break;
        }
    }
}

}; /* namespace interpreter::cpu */
