
#ifndef _MEMORY_H_INCLUDED_
#define _MEMORY_H_INCLUDED_

#include <lib/types.h>

namespace psx::memory {

/**
 * @brief Memory bus interface.
 * @details
 * Implements _CPU_ memory transactions. DMA and GPU accesses will _not_ use
 * this interface. Generated recompiled code will continue using this
 * ìnterface however.
 */
class Bus
{
public:
    Bus() {}
    virtual ~Bus() {}

    virtual bool load(unsigned bytes, uint32_t addr, uint32_t *val) = 0;
    virtual bool store(unsigned bytes, uint32_t addr, uint32_t val) = 0;

    inline bool load_u8(uint32_t addr, uint8_t *val) {
        uint32_t val32; bool res = load(1, addr, &val32);
        *val = val32; return res;
    }
    inline bool load_u16(uint32_t addr, uint16_t *val) {
        uint32_t val32; bool res = load(2, addr, &val32);
        *val = val32; return res;
    }
    inline bool load_u32(uint32_t addr, uint32_t *val) {
        return load(4, addr, val);
    }

    inline bool store_u8(uint32_t addr, uint8_t val) {
        return store(1, addr, val);
    }
    inline bool store_u16(uint32_t addr, uint16_t val) {
        return store(2, addr, val);
    }
    inline bool store_u32(uint32_t addr, uint32_t val) {
        return store(4, addr, val);
    }

    /**
     * @struct BusTransaction
     * @brief Record of a bus transaction.
     *
     * @var BusTransaction::load
     * @brief True if the access is a load, false otherwise.
     * @var BusTransaction::valid
     * @brief False if the transaction is invalid.
     * @var BusTransaction::bytes
     * @brief Bus access size in bytes (1, 2, 4, 8).
     * @var BusTransaction::address
     * @brief Physical memory address.
     * @var BusTransaction::value
     * @brief Bus access input or output value.
     */
    struct Transaction {
        bool load;
        bool valid;
        unsigned bytes;
        uint32_t address;
        uint32_t value;

        Transaction() :
            load(false), valid(false), bytes(0), address(0), value(0) {}
        Transaction(bool load, bool valid, unsigned bytes,
                    uint32_t address, uint32_t value) :
            load(load), valid(valid), bytes(bytes),
            address(address), value(value) {}
    };
};

// Helper to write a single half word to memory in little endian.
static void store_u16_le(uint8_t *ptr, uint16_t val) {
    ptr[1] = val >> 8;
    ptr[0] = val;
}

// Helper to write a single word to memory in little endian.
static void store_u32_le(uint8_t *ptr, uint32_t val) {
    ptr[3] = val >> 24;
    ptr[2] = val >> 16;
    ptr[1] = val >> 8;
    ptr[0] = val;
}

// Helper to read a single half word from memory in little endian.
static uint16_t load_u16_le(uint8_t const *ptr) {
    return ((uint32_t)ptr[1] << 8)  |
           ((uint32_t)ptr[0] << 0);
}

// Helper to read a single word from memory in little endian.
static uint32_t load_u32_le(uint8_t const *ptr) {
    return ((uint32_t)ptr[3] << 24) |
           ((uint32_t)ptr[2] << 16) |
           ((uint32_t)ptr[1] << 8)  |
           ((uint32_t)ptr[0] << 0);
}

}; /* namespace psx::memory */

#endif /* _MEMORY_H_INCLUDED_ */
