
#ifndef _ASSEMBLY_DISASSEMBLER_H_INCLUDED_
#define _ASSEMBLY_DISASSEMBLER_H_INCLUDED_

#include <string>
#include <lib/types.h>

namespace psx::assembly {
namespace cpu {

/** Disassemble and format a CPU instruction. */
std::string disassemble(uint32_t pc, uint32_t instr);

}; /* namespace cpu */

static inline uint32_t getOpcode(uint32_t instr) {
    return (instr >> 26) & 0x3flu;
}

static inline uint32_t getFmt(uint32_t instr) {
    return (instr >> 21) & 0x1flu;
}

static inline uint32_t getRs(uint32_t instr) {
    return (instr >> 21) & 0x1flu;
}

static inline uint32_t getRt(uint32_t instr) {
    return (instr >> 16) & 0x1flu;
}

static inline uint32_t getRd(uint32_t instr) {
    return (instr >> 11) & 0x1flu;
}

static inline uint32_t getFs(uint32_t instr) {
    return (instr >> 11) & 0x1flu;
}

static inline uint32_t getFt(uint32_t instr) {
    return (instr >> 16) & 0x1flu;
}

static inline uint32_t getFd(uint32_t instr) {
    return (instr >> 6) & 0x1flu;
}

static inline uint32_t getVt(uint32_t instr) {
    return (instr >> 16) & 0x1flu;
}

static inline uint32_t getVs(uint32_t instr) {
    return (instr >> 11) & 0x1flu;
}

static inline uint32_t getVd(uint32_t instr) {
    return (instr >> 6) & 0x1flu;
}

static inline uint32_t getElement(uint32_t instr) {
     return (instr >> 21) & 0xfu;
}

static inline uint32_t getShamnt(uint32_t instr) {
    return (instr >> 6) & 0x1flu;
}

static inline uint32_t getTarget(uint32_t instr) {
    return instr & 0x3fffffflu;
}

static inline uint32_t getImmediate(uint32_t instr) {
    return instr & 0xfffflu;
}

static inline uint32_t getFunct(uint32_t instr) {
    return instr & 0x3flu;
}

}; /* namespace psx::assembly */

#endif /* _ASSEMBLY_DISASSEMBLER_H_INCLUDED_ */
