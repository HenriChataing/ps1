
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

struct cp2_registers {
    // Data Registers.
    union {
        struct {
        uint32_t vxy0;
        uint32_t vz0;
        uint32_t vxy1;
        uint32_t vz1;
        uint32_t vxy2;
        uint32_t vz2;
        uint32_t rgbc;
        uint32_t otz;
        uint32_t ir0;
        uint32_t ir1;
        uint32_t ir2;
        uint32_t ir3;
        uint32_t sxy0;
        uint32_t sxy1;
        uint32_t sxy2;
        uint32_t sxyp;
        uint32_t sz0;
        uint32_t sz1;
        uint32_t sz2;
        uint32_t sz3;
        uint32_t rgb0;
        uint32_t rgb1;
        uint32_t rgb2;
        uint32_t res1;
        uint32_t mac0;
        uint32_t mac1;
        uint32_t mac2;
        uint32_t mac3;
        uint32_t irgb;
        uint32_t orgb;
        uint32_t lzcs;
        uint32_t lzcr;
        };
        uint32_t dr[32];
    };

    // Control Registers.
    uint32_t cr[32];
#if 0
  cop2r32-36 9xS16 RT11RT12,..,RT33 Rotation matrix     (3x3)        ;cnt0-4
  cop2r37-39 3x 32 TRX,TRY,TRZ      Translation vector  (X,Y,Z)      ;cnt5-7
  cop2r40-44 9xS16 L11L12,..,L33    Light source matrix (3x3)        ;cnt8-12
  cop2r45-47 3x 32 RBK,GBK,BBK      Background color    (R,G,B)      ;cnt13-15
  cop2r48-52 9xS16 LR1LR2,..,LB3    Light color matrix source (3x3)  ;cnt16-20
  cop2r53-55 3x 32 RFC,GFC,BFC      Far color           (R,G,B)      ;cnt21-23
  cop2r56-57 2x 32 OFX,OFY          Screen offset       (X,Y)        ;cnt24-25
  cop2r58 BuggyU16 H                Projection plane distance.       ;cnt26
  cop2r59      S16 DQA              Depth queing parameter A (coeff) ;cnt27
  cop2r60       32 DQB              Depth queing parameter B (offset);cnt28
  cop2r61-62 2xS16 ZSF3,ZSF4        Average Z scale factors          ;cnt29-30
  cop2r63      U20 FLAG             Returns any calculation errors   ;cnt31
#endif
};

// Check that register aliases are correctly packed.
static_assert(sizeof(cp2_registers) == (64 * sizeof(uint32_t)));

struct cdrom_registers {
    uint8_t index;
    uint8_t command;
    uint8_t request;

    uint8_t interrupt_flag;
    uint8_t interrupt_enable;

    uint8_t parameter_fifo[16];
    unsigned parameter_fifo_index;

    uint8_t response_fifo[16];
    unsigned response_fifo_length;
    unsigned response_fifo_index;

    uint8_t stat;
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

    uint16_t horizontal_display_range_x1;
    uint16_t horizontal_display_range_x2;
    uint16_t vertical_display_range_y1;
    uint16_t vertical_display_range_y2;

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
    union {
        uint32_t buffer[16];
        struct {
            uint32_t x0;
            uint32_t y0;
            uint32_t width;
            uint32_t height;
            uint32_t x;
            uint32_t y;
        } transfer;
    };
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
    struct cp2_registers cp2;
    struct hw_registers hw;
    struct cdrom_registers cdrom;
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
