
#ifndef _PSX_H_INCLUDED_
#define _PSX_H_INCLUDED_

#include <cstdint>
#include <mutex>
#include <psx/memory.h>

namespace psx {

enum cpu_state {
    Continue,   /**< Evaluate the instruction at pc+4 */
    Delay,      /**< Evaluate the instruction at pc+4, then perform a jump */
    Jump,       /**< Jump to the specified address. */
};

enum cpu_exception {
    None = 0,
    AddressError,
    TLBRefill,
    TLBInvalid,
    TLBModified,
    CacheError,
    VirtualCoherency,
    BusError,
    IntegerOverflow,
    SystemCall,
    Breakpoint,
    ReservedInstruction,
    CoprocessorUnusable,
    Interrupt,
};

struct cpu_registers {
    uint32_t pc;
    uint32_t gpr[32];
    uint32_t mult_hi;
    uint32_t mult_lo;
};

struct cp0_registers {
    uint32_t bpc;
    uint32_t bda;
    uint32_t jumpdest;
    uint32_t dcic;
    uint32_t badvaddr;
    uint32_t bdam;
    uint32_t bpcm;
    uint32_t sr;
    uint32_t cause;
    uint32_t epc;
    uint32_t prid;

#define STATUS_CU3              (UINT32_C(1) << 31)
#define STATUS_CU2              (UINT32_C(1) << 30)
#define STATUS_CU1              (UINT32_C(1) << 29)
#define STATUS_CU0              (UINT32_C(1) << 28)
#define STATUS_RE               (UINT32_C(1) << 25)
#define STATUS_BEV              (UINT32_C(1) << 22)
#define STATUS_TS               (UINT32_C(1) << 21)
#define STATUS_IC               (UINT32_C(1) << 16)
#define STATUS_KUo              (UINT32_C(1) << 5)
#define STATUS_IEo              (UINT32_C(1) << 4)
#define STATUS_KUp              (UINT32_C(1) << 3)
#define STATUS_IEp              (UINT32_C(1) << 2)
#define STATUS_KUc              (UINT32_C(1) << 1)
#define STATUS_IEc              (UINT32_C(1) << 0)

    inline bool CU3() { return (sr & STATUS_CU3) != 0; }
    inline bool CU2() { return (sr & STATUS_CU2) != 0; }
    inline bool CU1() { return (sr & STATUS_CU1) != 0; }
    inline bool CU0() { return (sr & STATUS_CU0) != 0; }
    inline bool RE()  { return (sr & STATUS_RE) != 0; }
    inline bool BEV() { return (sr & STATUS_BEV) != 0; }
    inline bool TS()  { return (sr & STATUS_TS) != 0; }
    inline bool IC()  { return (sr & STATUS_IC) != 0; }
    inline uint32_t IM()   { return (sr >> 8) & 0xfflu; }
    inline bool KUo() { return (sr & STATUS_KUo) != 0; }
    inline bool IEo() { return (sr & STATUS_IEo) != 0; }
    inline bool KUp() { return (sr & STATUS_KUp) != 0; }
    inline bool IEp() { return (sr & STATUS_IEp) != 0; }
    inline bool KUc() { return (sr & STATUS_KUc) != 0; }
    inline bool IEc() { return (sr & STATUS_IEc) != 0; }

#define CAUSE_BD                (UINT32_C(1) << 31)
#define CAUSE_CE_MASK           (UINT32_C(0x3) << 28)
#define CAUSE_CE(ce)            ((uint32_t)(ce) << 28)
#define CAUSE_IP_MASK           (UINT32_C(0xff) << 8)
#define CAUSE_IP(ip)            ((uint32_t)(ip) << 8)
#define CAUSE_IP2               (UINT32_C(1) << 10)
#define CAUSE_EXCCODE_MASK      (UINT32_C(0x1f) << 2)
#define CAUSE_EXCCODE(exccode)  ((uint32_t)(exccode) << 2)

    inline uint32_t IP()   { return (cause >> 8) & 0xfflu; }
};

struct gpu_registers {
    uint8_t dma_direction;

    bool display_enable;
    bool vertical_interlace;
    uint8_t horizontal_resolution;
    uint8_t vertical_resolution;
    uint8_t video_mode;
    uint8_t display_area_color_depth;

    uint16_t start_of_display_area_x;
    uint16_t start_of_display_area_y;

    uint32_t horizontal_display_range;
    uint32_t vertical_display_range;

    bool texture_disable;
    bool dither_enable;
    bool drawing_to_display_area_enable;
    uint8_t semi_transparency_mode;
    bool force_bit_mask;
    bool check_bit_mask;

    uint8_t texture_page_x_base;
    uint8_t texture_page_y_base;
    uint8_t texture_page_colors;
    bool textured_rectangle_x_flip;
    bool textured_rectangle_y_flip;
    uint8_t texture_window_mask_x;
    uint8_t texture_window_mask_y;
    uint8_t texture_window_offset_x;
    uint8_t texture_window_offset_y;

    int16_t drawing_area_x1;
    int16_t drawing_area_y1;
    int16_t drawing_area_x2;
    int16_t drawing_area_y2;
    int16_t drawing_offset_x;
    int16_t drawing_offset_y;

    uint32_t gp0_buffer[16];
    unsigned gp0_buffer_index;
    unsigned long hblank_clock;
    unsigned long dot_clock;
    unsigned scanline;
    unsigned frame;
};

enum gp0_state {
    GP0_COMMAND,            ///< Executing a generic command.
    GP0_POLYLINE,           ///< Executing a polyline render command.
    GP0_COPY_CPU_TO_VRAM,   ///< Executing a transfer from CPU to VRAM.
    GP0_COPY_VRAM_TO_CPU,   ///< Executing a transfer from VRAM to CPU.
};

struct gp0_registers {
    uint32_t buffer[16];
    uint32_t count;
    enum gp0_state state;
};

struct hw_registers {
    // Memory Control
    uint32_t expansion_1_base_addr;
    uint32_t expansion_2_base_addr;
    uint32_t expansion_1_delay_size;
    uint32_t expansion_3_delay_size;
    uint32_t bios_rom_delay_size;
    uint32_t spu_delay;
    uint32_t cdrom_delay;
    uint32_t expansion_2_delay_size;
    uint32_t common_delay;
    uint32_t ram_size;
    uint32_t cache_control;

    // Peripheral I/O Ports
    uint32_t joy_stat;
    uint16_t joy_mode;
    uint16_t joy_ctrl;
    uint16_t joy_baud;
    uint32_t sio_stat;
    uint16_t sio_mode;
    uint16_t sio_ctrl;
    uint16_t sio_misc;
    uint16_t sio_baud;

    // Interrupt Control
    uint16_t i_stat;
    uint16_t i_mask;

    // DMA Registers
    struct {
        uint32_t madr;
        uint32_t bcr;
        uint32_t chcr;
    } dma[7];
    uint32_t dpcr;
    uint32_t dicr;

    // Timers
    struct {
      uint16_t value;
      uint16_t mode;
      uint16_t target;

      unsigned long last_counter_update;
    } timer[3];

    // SPU Control
    uint16_t main_volume_left;
    uint16_t main_volume_right;
    uint16_t reverb_output_volume_left;
    uint16_t reverb_output_volume_right;
    uint32_t voice_key_on;
    uint32_t voice_key_off;
    uint32_t voice_channel_fm;
    uint32_t voice_channel_noise_mode;
    uint32_t voice_channel_reverb_mode;
    uint32_t voice_channel_on;
    uint16_t sound_ram_reverb_start_addr;
    uint16_t sound_ram_irq_addr;
    uint16_t sound_ram_data_transfer_addr;
    uint16_t sound_ram_data_transfer_fifo;
    uint16_t spu_control;
    uint16_t sound_ram_data_transfer_control;
    uint16_t spu_status;
    uint16_t cd_volume_left;
    uint16_t cd_volume_right;
    uint16_t extern_volume_left;
    uint16_t extern_volume_right;
    uint16_t current_main_volume_left;
    uint16_t current_main_volume_right;

    // GPU
    uint32_t gpustat;
};

struct event {
    unsigned long timeout;
    void (*callback)();
    struct event *next;

    event(unsigned long timeout, void (*callback)(), struct event *next)
        : timeout(timeout), callback(callback), next(next) {};
};

struct state {
    struct cpu_registers cpu;
    struct cp0_registers cp0;
    struct hw_registers hw;
    struct gpu_registers gpu;
    struct gp0_registers gp0;

    uint8_t ram[0x200000];
    uint8_t bios[0x80000];
    uint8_t dram[0x400];
    uint8_t vram[0x100000];
    uint8_t *cd_rom;
    size_t cd_rom_size;

    unsigned long cycles;
    enum cpu_state cpu_state;
    uint32_t jump_address;
    unsigned long next_event;
    struct event *event_queue;
    bool delay_slot;
    psx::memory::Bus *bus;

    state();
    ~state();

    int load_bios(std::istream &bios_contents);
    int load_cd_rom(std::istream &cd_rom_contents);
    void reset();

    void handle_event();
    void schedule_event(unsigned long timeout, void (*callback)());
    void cancel_event(void (*callback)());
    void cancel_all_events(void);

private:
    /** Mutex to control access to concurrent resources. */
    std::mutex _mutex;
};

/** Machine state. */
extern state state;

/**
 * @brief Start the interpreter and recompiler in separate threads.
 * The interpreter is initially halted and should be kicked off with
 * \ref psx::resume().
 */
void start(void);

/**
 * @brief Kill the interpreter and recompiler threads.
 * The function first halts the interpreter for a clean exit.
 */
void stop(void);

/** Reset the machine state. */
void reset(void);

/** Halt the interpreter for the given reason. */
void halt(std::string reason);
bool halted(void);
std::string halted_reason(void);

/** When the debugger is halted, advance the interpreter one step */
void step(void);

/** When the debugger is halted, resume execution. */
void resume(void);

void set_interrupt_pending(unsigned irq);
void clear_interrupt_pending(unsigned irq);
void check_interrupt(void);

void take_exception(
    cpu_exception exn, uint32_t virt_addr, bool instr,
    bool load, uint32_t ce = 0);

cpu_exception translate_address(
    uint32_t virt_addr, uint32_t *phys_addr, bool write_access,
    uint32_t *virt_start = NULL, uint32_t *virt_end = NULL);

class DefaultBus: public psx::memory::Bus {
public:
    DefaultBus() {}
    ~DefaultBus() {}
    virtual bool load(unsigned bytes, uint32_t addr, uint32_t *val);
    virtual bool store(unsigned bytes, uint32_t addr, uint32_t val);
};

}; /* PSX */

#endif /* _PSX_H_INCLUDED_ */
