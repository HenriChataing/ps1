
#ifndef _ASSEMBLY_REGISTERS_H_INCLUDED_
#define _ASSEMBLY_REGISTERS_H_INCLUDED_

namespace psx::assembly::cpu {

enum Cop0Register {
    // Standard registers.
    BadVAddr = 8,
    SR = 12,
    Cause = 13,
    EPC = 14,
    PrId = 15,
    // Debug registers.
    BPC = 3,
    BDA = 5,
    JUMPDEST = 6,
    DCIC = 7,
    BDAM = 9,
    BPCM = 11,
};

extern const char *RegisterNames[32];
extern const char *Cop0RegisterNames[32];

static inline const char *getRegisterName(unsigned nr) {
    return nr >= 32 ? "$?" : RegisterNames[nr];
}
static inline const char *getCop0RegisterName(unsigned nr) {
    return nr >= 32 ? "$c?" : Cop0RegisterNames[nr];
}

}; /* namespace psx::assembly::cpu */

#endif /* _ASSEMBLY_REGISTERS_H_INCLUDED_ */
