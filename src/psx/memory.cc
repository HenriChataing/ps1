
#include <fmt/format.h>

#include <psx/debugger.h>
#include <psx/memory.h>
#include <psx/psx.h>
#include <psx/hw.h>

using namespace psx;

static uint32_t load_le(uint8_t const *ptr, unsigned bytes) {
    switch (bytes) {
    case 1: return ((uint32_t)ptr[0] << 0);
    case 2: return memory::load_u16_le(ptr);
    case 4: return memory::load_u32_le(ptr);
    default: return 0;
    }
}

static void store_le(uint8_t *ptr, unsigned bytes, uint32_t val) {
    switch (bytes) {
    case 1:
        ptr[0] = val;
        break;
    case 2: memory::store_u16_le(ptr, val); break;
    case 4: memory::store_u32_le(ptr, val); break;
    default:
        break;
    }
}

static bool load_u8_(uint32_t addr, uint32_t *val) {
    switch (addr) {

    // CDROM Controller I/O Ports
    case UINT32_C(0x1f801800): hw::read_cdrom_index(val); break;
    case UINT32_C(0x1f801801): hw::read_cdrom_reg01(val); break;
    case UINT32_C(0x1f801803): hw::read_cdrom_reg03(val); break;

    // Controller and Memory Card I/O Ports
    case UINT32_C(0x1f801040): hw::read_joy_data(val); break;

    default:
        std::string reason = fmt::format("load_u8 at 0x{:08x}", addr);
        psx::halt(reason);
        return false;
    }

    return true;
}

static bool load_u16_(uint32_t addr, uint32_t *val) {
    switch (addr) {
    // Controller and Memory Card I/O Ports
    case UINT32_C(0x1f801044):  hw::read_joy_stat(val); break;
    case UINT32_C(0x1f801048):  hw::read_joy_mode(val); break;
    case UINT32_C(0x1f80104a):  hw::read_joy_ctrl(val); break;
    case UINT32_C(0x1f80104e):  hw::read_joy_baud(val); break;

    // Interrupt Control
    case UINT32_C(0x1f801070):  hw::read_i_stat(val); break;
    case UINT32_C(0x1f801074):  hw::read_i_mask(val); break;

    // Timers
    case UINT32_C(0x1f801120):  hw::read_timer_value(2, val); break;

    // SPU Control
    case UINT32_C(0x1f801d80):  *val = state.hw.main_volume_left; break;
    case UINT32_C(0x1f801d82):  *val = state.hw.main_volume_right; break;
    case UINT32_C(0x1f801d84):  *val = state.hw.main_volume_left; break;
    case UINT32_C(0x1f801d86):  *val = state.hw.main_volume_right; break;
    case UINT32_C(0x1f801d88):  *val = state.hw.voice_key_on; break; // TODO lo
    case UINT32_C(0x1f801d8a):  *val = state.hw.voice_key_on; break; // TODO hi
    case UINT32_C(0x1f801d8c):  *val = state.hw.voice_key_off; break; // TODO lo
    case UINT32_C(0x1f801d8e):  *val = state.hw.voice_key_off; break; // TODO hi
    case UINT32_C(0x1f801d90):  *val = state.hw.voice_channel_fm; break; // TODO lo
    case UINT32_C(0x1f801d92):  *val = state.hw.voice_channel_fm; break; // TODO hi
    case UINT32_C(0x1f801d94):  *val = state.hw.voice_channel_noise_mode; break; // TODO lo
    case UINT32_C(0x1f801d96):  *val = state.hw.voice_channel_noise_mode; break; // TODO hi
    case UINT32_C(0x1f801d98):  *val = state.hw.voice_channel_reverb_mode; break; // TODO lo
    case UINT32_C(0x1f801d9a):  *val = state.hw.voice_channel_reverb_mode; break; // TODO hi
    case UINT32_C(0x1f801da2):  *val = state.hw.sound_ram_reverb_start_addr; break;
    case UINT32_C(0x1f801da6):  *val = state.hw.sound_ram_data_transfer_addr; break;
    case UINT32_C(0x1f801da8):  *val = state.hw.sound_ram_data_transfer_fifo; break;
    case UINT32_C(0x1f801daa):  *val = state.hw.spu_control; break;
    case UINT32_C(0x1f801dac):  *val = state.hw.sound_ram_data_transfer_control; break;
    case UINT32_C(0x1f801dae):  *val = state.hw.spu_status; break;
    case UINT32_C(0x1f801db0):  *val = state.hw.cd_volume_left; break;
    case UINT32_C(0x1f801db2):  *val = state.hw.cd_volume_right; break;
    case UINT32_C(0x1f801db4):  *val = state.hw.extern_volume_left; break;
    case UINT32_C(0x1f801db6):  *val = state.hw.extern_volume_right; break;

    default:
        std::string reason = fmt::format("load_u16 at 0x{:08x}", addr);
        psx::halt(reason);
        return false;
    }

    return true;
}

static bool load_u32_(uint32_t addr, uint32_t *val) {
    switch (addr) {
    // Interrupt Control
    case UINT32_C(0x1f801070):  hw::read_i_stat(val); break;
    case UINT32_C(0x1f801074):  hw::read_i_mask(val); break;

    // Timer Control
    case UINT32_C(0x1f801110):  hw::read_timer_value(1, val); break;

    // DMA Control
    case UINT32_C(0x1f8010a0):  hw::read_dx_madr(2, val); break;
    case UINT32_C(0x1f8010a8):  hw::read_dx_chcr(2, val); break;
    case UINT32_C(0x1f8010e8):  hw::read_dx_chcr(6, val); break;
    case UINT32_C(0x1f8010f0):  hw::read_dpcr(val); break;
    case UINT32_C(0x1f8010f4):  hw::read_dicr(val); break;

    // GPU I/O Ports
    case UINT32_C(0x1f801810):  hw::read_gpuread(val); break;
    case UINT32_C(0x1f801814):  hw::read_gpustat(val); break;

    default:
        std::string reason = fmt::format("load_u32 at 0x{:08x}", addr);
        psx::halt(reason);
        return false;
    }

    return true;
}

bool DefaultBus::load(unsigned bytes, uint32_t addr, uint32_t *val) {
    if (addr < UINT32_C(0x200000)) {
        *val = load_le(state.ram + addr, bytes);
        return true;
    }

    if (addr >= UINT32_C(0x1F800000) &&
        addr <  UINT32_C(0x1F800400)) {
        *val = load_le(state.dram + (addr - UINT32_C(0x1F800000)), bytes);
        return true;
    }

    if (addr >= UINT32_C(0x1FC00000) &&
        addr <  UINT32_C(0x1FC80000)) {
        *val = load_le(state.bios + (addr - UINT32_C(0x1FC00000)), bytes);
        return true;
    }

    if (addr >= UINT32_C(0x1F000000) &&
        addr <  UINT32_C(0x1F000100)) {
        *val = load_le(state.cd_rom + (addr - UINT32_C(0x1F000000)), bytes);
        return true;
    }

    if (addr >= UINT32_C(0x1f801c00) &&
        addr <  UINT32_C(0x1f801d80)) {
        // TODO SPU Voice
        *val = 0;
        return true;
    }

    switch (bytes) {
    case 1: return load_u8_(addr, val);
    case 2: return load_u16_(addr, val);
    case 4: return load_u32_(addr, val);
    default: return false;
    }
    return false;
}

static bool store_u8_(uint32_t addr, uint8_t val) {
    switch (addr) {
    // Controller and Memory Card I/O Ports
    case UINT32_C(0x1f801040): hw::write_joy_data(val); break;

    // Expansion Region 2 - Int/Dip/Post
    // POST external 7 segment display, indicate BIOS boot status
    case UINT32_C(0x1f802041): break;

    // CDROM Controller I/O Ports
    case UINT32_C(0x1f801800): hw::write_cdrom_index(val); break;
    case UINT32_C(0x1f801801): hw::write_cdrom_reg01(val); break;
    case UINT32_C(0x1f801802): hw::write_cdrom_reg02(val); break;
    case UINT32_C(0x1f801803): hw::write_cdrom_reg03(val); break;

    default:
        std::string reason = fmt::format("store_u8 at 0x{:08x}", addr);
        psx::halt(reason);
        return false;
    }

    return true;
}

static bool store_u16_(uint32_t addr, uint16_t val) {
    switch (addr) {
    // Controller and Memory Card I/O Ports
    case UINT32_C(0x1f801048):  hw::write_joy_mode(val); break;
    case UINT32_C(0x1f80104a):  hw::write_joy_ctrl(val); break;
    case UINT32_C(0x1f80104e):  hw::write_joy_baud(val); break;

    // Interrupt Control
    case UINT32_C(0x1f801070):  hw::write_i_stat(val); break;
    case UINT32_C(0x1f801074):  hw::write_i_mask(val); break;

    // Timers
    case UINT32_C(0x1f801100):  hw::write_timer_value(0, val); break;
    case UINT32_C(0x1f801104):  hw::write_timer_mode(0, val); break;
    case UINT32_C(0x1f801108):  hw::write_timer_target(0, val); break;
    case UINT32_C(0x1f801110):  hw::write_timer_value(1, val); break;
    case UINT32_C(0x1f801114):  hw::write_timer_mode(1, val); break;
    case UINT32_C(0x1f801118):  hw::write_timer_target(1, val); break;
    case UINT32_C(0x1f801120):  hw::write_timer_value(2, val); break;
    case UINT32_C(0x1f801124):  hw::write_timer_mode(2, val); break;
    case UINT32_C(0x1f801128):  hw::write_timer_target(2, val); break;

    // SPU Control
    case UINT32_C(0x1f801d80):  state.hw.main_volume_left = val; break;
    case UINT32_C(0x1f801d82):  state.hw.main_volume_right = val; break;
    case UINT32_C(0x1f801d84):  state.hw.main_volume_left = val; break;
    case UINT32_C(0x1f801d86):  state.hw.main_volume_right = val; break;
    case UINT32_C(0x1f801d88):  state.hw.voice_key_on = val; break; // TODO lo
    case UINT32_C(0x1f801d8a):  state.hw.voice_key_on = val; break; // TODO hi
    case UINT32_C(0x1f801d8c):  state.hw.voice_key_off = val; break; // TODO lo
    case UINT32_C(0x1f801d8e):  state.hw.voice_key_off = val; break; // TODO hi
    case UINT32_C(0x1f801d90):  state.hw.voice_channel_fm = val; break; // TODO lo
    case UINT32_C(0x1f801d92):  state.hw.voice_channel_fm = val; break; // TODO hi
    case UINT32_C(0x1f801d94):  state.hw.voice_channel_noise_mode = val; break; // TODO lo
    case UINT32_C(0x1f801d96):  state.hw.voice_channel_noise_mode = val; break; // TODO hi
    case UINT32_C(0x1f801d98):  state.hw.voice_channel_reverb_mode = val; break; // TODO lo
    case UINT32_C(0x1f801d9a):  state.hw.voice_channel_reverb_mode = val; break; // TODO hi
    case UINT32_C(0x1f801da2):  state.hw.sound_ram_reverb_start_addr = val; break;
    case UINT32_C(0x1f801da6):  state.hw.sound_ram_data_transfer_addr = val; break;
    case UINT32_C(0x1f801da8):  state.hw.sound_ram_data_transfer_fifo = val; break;
    case UINT32_C(0x1f801daa):  state.hw.spu_control = val; break;
    case UINT32_C(0x1f801dae):  state.hw.spu_status = val; break;
    case UINT32_C(0x1f801dac):  state.hw.sound_ram_data_transfer_control = val; break;
    case UINT32_C(0x1f801db0):  state.hw.cd_volume_left = val; break;
    case UINT32_C(0x1f801db2):  state.hw.cd_volume_right = val; break;
    case UINT32_C(0x1f801db4):  state.hw.extern_volume_left = val; break;
    case UINT32_C(0x1f801db6):  state.hw.extern_volume_right = val; break;

    default:
        std::string reason = fmt::format("store_u16 at 0x{:08x}", addr);
        psx::halt(reason);
        return false;
    }

    return true;
}

static bool store_u32_(uint32_t addr, uint32_t val) {
    switch (addr) {
    // Memory Control
    case UINT32_C(0x1f801000):  state.hw.expansion_1_base_addr = val; break;
    case UINT32_C(0x1f801004):  state.hw.expansion_2_base_addr = val; break;
    case UINT32_C(0x1f801008):  state.hw.expansion_1_delay_size = val; break;
    case UINT32_C(0x1f80100c):  state.hw.expansion_3_delay_size = val; break;
    case UINT32_C(0x1f801010):  state.hw.bios_rom_delay_size = val; break;
    case UINT32_C(0x1f801014):  state.hw.spu_delay = val; break;
    case UINT32_C(0x1f801018):  state.hw.cdrom_delay = val; break;
    case UINT32_C(0x1f80101c):  state.hw.expansion_2_delay_size = val; break;
    case UINT32_C(0x1f801020):  state.hw.common_delay = val; break;
    case UINT32_C(0x1f801060):  state.hw.ram_size = val; break;
    case UINT32_C(0xfffe0130):  state.hw.cache_control = val; break;

    // Interrupt Control
    case UINT32_C(0x1f801070):  hw::write_i_stat(val); break;
    case UINT32_C(0x1f801074):  hw::write_i_mask(val); break;

    // DMA Control
    case UINT32_C(0x1f8010a0):  hw::write_dx_madr(2, val); break;
    case UINT32_C(0x1f8010a4):  hw::write_dx_bcr(2, val); break;
    case UINT32_C(0x1f8010a8):  hw::write_d2_chcr(val); break;
    case UINT32_C(0x1f8010e0):  hw::write_dx_madr(6, val); break;
    case UINT32_C(0x1f8010e4):  hw::write_dx_bcr(6, val); break;
    case UINT32_C(0x1f8010e8):  hw::write_d6_chcr(val); break;
    case UINT32_C(0x1f8010f0):  hw::write_dpcr(val); break;
    case UINT32_C(0x1f8010f4):  hw::write_dicr(val); break;

    // GPU I/O Ports
    case UINT32_C(0x1f801810):  hw::write_gpu0(val); break;
    case UINT32_C(0x1f801814):  hw::write_gpu1(val); break;

    // Garbage I/O Locations
    case UINT32_C(0x1f801114):
    case UINT32_C(0x1f801118):
        break;

    default:
        std::string reason = fmt::format("store_u32 at 0x{:08x}", addr);
        psx::halt(reason);
        return false;
    }

    return true;
}

bool DefaultBus::store(unsigned bytes, uint32_t addr, uint32_t val) {
    if (state.cp0.IC()) {
        // When the Isolate Cache bit is set in the Status register,
        // store operations do not propagate to the external memory system.
        // The BIOS sets this bit in order to clear the instruction
        // cache in the early boot process.
        return true;
    }

    if (addr < UINT32_C(0x200000)) {
        store_le(state.ram + addr, bytes, val);
        return true;
    }

    if (addr >= UINT32_C(0x1f800000) &&
        addr <  UINT32_C(0x1f800400)) {
        store_le(state.dram + (addr - UINT32_C(0x1f800000)), bytes, val);
        return true;
    }

    if (addr >= UINT32_C(0x1fc00000) &&
        addr <  UINT32_C(0x1fc80000)) {
        return false;
    }

    if (addr >= UINT32_C(0x1f801c00) &&
        addr <  UINT32_C(0x1f801d80)) {
        // TODO SPU Voice
        return true;
    }

    if (addr >= UINT32_C(0x1f801dc0) &&
        addr <  UINT32_C(0x1f801e00)) {
        // TODO SPU Reverb Config
        return true;
    }

    switch (bytes) {
    case 1: return store_u8_(addr, val);
    case 2: return store_u16_(addr, val);
    case 4: return store_u32_(addr, val);
    default: return false;
    }
}
