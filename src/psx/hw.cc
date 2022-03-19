
#include <psx/psx.h>
#include <psx/hw.h>
#include <psx/debugger.h>
#include <psx/memory.h>

using namespace psx;

namespace psx::hw {

void read_joy_data(uint32_t *val) {
    // TODO
    *val = 0;
    debugger::debug(Debugger::JC, "joy_data -> {:02x}", *val);
}

//  0     TX Ready Flag 1   (1=Ready/Started)
//  1     RX FIFO Not Empty (0=Empty, 1=Not Empty)
//  2     TX Ready Flag 2   (1=Ready/Finished)
//  3     RX Parity Error   (0=No, 1=Error; Wrong Parity, when enabled)  (sticky)
//  4     Unknown (zero)    (unlike SIO, this isn't RX FIFO Overrun flag)
//  5     Unknown (zero)    (for SIO this would be RX Bad Stop Bit)
//  6     Unknown (zero)    (for SIO this would be RX Input Level AFTER Stop bit)
//  7     /ACK Input Level  (0=High, 1=Low)
//  8     Unknown (zero)    (for SIO this would be CTS Input Level)
//  9     Interrupt Request (0=None, 1=IRQ7) (See JOY_CTRL.Bit4,10-12)   (sticky)
//  10    Unknown (always zero)
//  11-31 Baudrate Timer    (21bit timer, decrementing at 33MHz)

#define JOY_STAT_INT                (UINT32_C(1) << 9)
#define JOY_STAT_ACK_INPUT_LEVEL    (UINT32_C(1) << 7)
#define JOY_STAT_RX_PARITY_ERROR    (UINT32_C(1) << 3)
#define JOY_STAT_TX_READY_2         (UINT32_C(1) << 2)
#define JOY_STAT_RX_FIFO_NOT_EMPTY  (UINT32_C(1) << 1)
#define JOY_STAT_TX_READY_1         (UINT32_C(1) << 0)

void read_joy_stat(uint32_t *val) {
    *val = state.hw.joy_stat;
    debugger::info(Debugger::JC, "joy_stat -> {:02x}", *val);
}

void read_joy_mode(uint32_t *val) {
    *val = state.hw.joy_mode;
    debugger::info(Debugger::JC, "joy_mode -> {:02x}", *val);
}

void read_joy_ctrl(uint32_t *val) {
    *val = state.hw.joy_ctrl;
    debugger::info(Debugger::JC, "joy_ctrl -> {:02x}", *val);
}

void read_joy_baud(uint32_t *val) {
    *val = state.hw.joy_baud;
    debugger::info(Debugger::JC, "joy_baud -> {:02x}", *val);
}

void write_joy_mode(uint16_t val) {
    debugger::info(Debugger::JC, "joy_mode <- {:04x}", val);
    state.hw.joy_mode = val;
}

void write_joy_ctrl(uint16_t val) {
    debugger::info(Debugger::JC, "joy_ctrl <- {:04x}", val);
    state.hw.joy_ctrl = val;
}

void write_joy_baud(uint16_t val) {
    debugger::info(Debugger::JC, "joy_baud <- {:04x}", val);
    state.hw.joy_baud = val;
}

static void check_ip2(void) {
    bool any_set = (state.hw.i_stat & state.hw.i_mask) != 0;
    if (any_set) {
        state.cp0.cause |= CAUSE_IP2;
    } else {
        state.cp0.cause &= ~CAUSE_IP2;
    }
    check_interrupt();
}

void set_i_stat(uint32_t irq) {
    state.hw.i_stat |= irq;
    check_ip2();
}

void read_i_stat(uint32_t *val) {
    *val = state.hw.i_stat;
    debugger::debug(Debugger::IC, "i_stat -> {:04x}", *val);
}

void read_i_mask(uint32_t *val) {
    *val = state.hw.i_mask;
    debugger::debug(Debugger::IC, "i_mask -> {:04x}", *val);
}

void write_i_stat(uint32_t val) {
    debugger::debug(Debugger::IC, "i_stat <- {:04x}", val);
    state.hw.i_stat &= val;
    check_ip2();
}

void write_i_mask(uint32_t val) {
    debugger::debug(Debugger::IC, "i_mask <- {:04x}", val);
    state.hw.i_mask = val;
    check_ip2();
}

#define TIMER_MODE_EQ_MAX       (UINT32_C(1) << 12)
#define TIMER_MODE_EQ_TARGET    (UINT32_C(1) << 11)
#define TIMER_MODE_INT_DISABLE  (UINT32_C(1) << 10)
#define TIMER_MODE_INT_TOGGLE   (UINT32_C(1) << 7)
#define TIMER_MODE_INT_REPEAT   (UINT32_C(1) << 6)
#define TIMER_MODE_INT_MAX      (UINT32_C(1) << 5)
#define TIMER_MODE_INT_TARGET   (UINT32_C(1) << 4)
#define TIMER_MODE_RST_TARGET   (UINT32_C(1) << 3)
#define TIMER_MODE_SYNC_ENABLE  (UINT32_C(1) << 0)

//  0     Synchronization Enable (0=Free Run, 1=Synchronize via Bit1-2)
//  1-2   Synchronization Mode   (0-3, see lists below)
//         Synchronization Modes for Counter 0:
//           0 = Pause counter during Hblank(s)
//           1 = Reset counter to 0000h at Hblank(s)
//           2 = Reset counter to 0000h at Hblank(s) and pause outside of Hblank
//           3 = Pause until Hblank occurs once, then switch to Free Run
//         Synchronization Modes for Counter 1:
//           Same as above, but using Vblank instead of Hblank
//         Synchronization Modes for Counter 2:
//           0 or 3 = Stop counter at current value (forever, no h/v-blank start)
//           1 or 2 = Free Run (same as when Synchronization Disabled)
//  3     Reset counter to 0000h  (0=After Counter=FFFFh, 1=After Counter=Target)
//  4     IRQ when Counter=Target (0=Disable, 1=Enable)
//  5     IRQ when Counter=FFFFh  (0=Disable, 1=Enable)
//  6     IRQ Once/Repeat Mode    (0=One-shot, 1=Repeatedly)
//  7     IRQ Pulse/Toggle Mode   (0=Short Bit10=0 Pulse, 1=Toggle Bit10 on/off)
//  8-9   Clock Source (0-3, see list below)
//         Counter 0:  0 or 2 = System Clock,  1 or 3 = Dotclock
//         Counter 1:  0 or 2 = System Clock,  1 or 3 = Hblank
//         Counter 2:  0 or 1 = System Clock,  2 or 3 = System Clock/8
//  10    Interrupt Request       (0=Yes, 1=No) (Set after Writing)    (W=1) (R)
//  11    Reached Target Value    (0=No, 1=Yes) (Reset after Reading)        (R)
//  12    Reached FFFFh Value     (0=No, 1=Yes) (Reset after Reading)        (R)

static void timer0_event() {
}

static void timer1_event() {
}

static void timer2_event() {
}

static void (*timer_event[3])() = {
    timer0_event,
    timer1_event,
    timer2_event,
};

void read_timer_value(int timer, uint32_t *val) {
    *val = state.cycles & UINT64_C(0xffff);
    debugger::info(Debugger::Timer, "tim{}_value -> {:04x}",
        timer, *val);
}

void write_timer_value(int timer, uint16_t val) {
    debugger::info(Debugger::Timer, "tim{}_value <- {:04x}", timer, val);
    state.cancel_event(timer_event[timer]);
    state.hw.timer[timer].value = 0;
    state.hw.timer[timer].last_counter_update = state.cycles;
/*    state.schedule_event();*/
}

void write_timer_mode(int timer, uint16_t val) {
    debugger::info(Debugger::Timer, "tim{}_mode <- {:04x}", timer, val);
    state.hw.timer[timer].mode &= ~UINT32_C(0x3ff);
    state.hw.timer[timer].mode |= val & UINT32_C(0x3ff);
}

void write_timer_target(int timer, uint16_t val) {
    debugger::info(Debugger::Timer, "tim{}_target <- {:04x}", timer, val);
    state.hw.timer[timer].target = 0;
}

//  0-2   DMA0, MDECin  Priority      (0..7; 0=Highest, 7=Lowest)
//  3     DMA0, MDECin  Master Enable (0=Disable, 1=Enable)
//  4-6   DMA1, MDECout Priority      (0..7; 0=Highest, 7=Lowest)
//  7     DMA1, MDECout Master Enable (0=Disable, 1=Enable)
//  8-10  DMA2, GPU     Priority      (0..7; 0=Highest, 7=Lowest)
//  11    DMA2, GPU     Master Enable (0=Disable, 1=Enable)
//  12-14 DMA3, CDROM   Priority      (0..7; 0=Highest, 7=Lowest)
//  15    DMA3, CDROM   Master Enable (0=Disable, 1=Enable)
//  16-18 DMA4, SPU     Priority      (0..7; 0=Highest, 7=Lowest)
//  19    DMA4, SPU     Master Enable (0=Disable, 1=Enable)
//  20-22 DMA5, PIO     Priority      (0..7; 0=Highest, 7=Lowest)
//  23    DMA5, PIO     Master Enable (0=Disable, 1=Enable)
//  24-26 DMA6, OTC     Priority      (0..7; 0=Highest, 7=Lowest)
//  27    DMA6, OTC     Master Enable (0=Disable, 1=Enable)

//  0-5   Unknown  (read/write-able)
//  6-14  Not used (always zero)
//  15    Force IRQ (sets bit31)            (0=None, 1=Force Bit31=1)
//  16-22 IRQ Enable for DMA0..DMA6         (0=None, 1=Enable)
//  23    IRQ Master Enable for DMA0..DMA6  (0=None, 1=Enable)
//  24-30 IRQ Flags for DMA0..DMA6          (0=None, 1=IRQ)    (Write 1 to reset)
//  31    IRQ Master Flag                   (0=None, 1=IRQ)    (Read only)

#define DICR_FORCE_IRQ          (UINT32_C(1) << 15)
#define DICR_IRQ_ENABLE(ch)     (UINT32_C(1) << (16 + (ch)))
#define DICR_IRQ_MASTER_ENABLE  (UINT32_C(1) << 23)
#define DICR_IRQ_FLAG(ch)       (UINT32_C(1) << (24 + (ch)))
#define DICR_IRQ_MASTER_FLAG    (UINT32_C(1) << 31)

static void check_dicr_irq_master_flag() {
    uint32_t irq_enable = (state.hw.dicr >> 16) & UINT32_C(0x7f);
    uint32_t irq_flag = (state.hw.dicr >> 24) & UINT32_C(0x7f);
    bool set_before = (state.hw.dicr & DICR_IRQ_MASTER_FLAG) != 0;
    bool set =
        (state.hw.dicr & DICR_FORCE_IRQ) != 0 ||
       ((state.hw.dicr & DICR_IRQ_MASTER_ENABLE) != 0 &&
        (irq_enable & irq_flag) != 0);
    if (set) {
        state.hw.dicr |= DICR_IRQ_MASTER_FLAG;
    } else {
        state.hw.dicr &= ~DICR_IRQ_MASTER_FLAG;
    }

    if (set && !set_before) {
        set_i_stat(I_STAT_DMA);
    }
}

void write_dpcr(uint32_t val) {
    debugger::info(Debugger::DMA, "dpcr <- {:08x}", val);
    state.hw.dpcr = val;
}

void write_dicr(uint32_t val) {
    debugger::info(Debugger::DMA, "dicr <- {:08x}", val);
    state.hw.dicr &= ~UINT32_C(0xff0000);
    state.hw.dicr |=   val & UINT32_C(0x00ff0000);
    state.hw.dicr &= ~(val & UINT32_C(0x7f000000));
    check_dicr_irq_master_flag();
}

//  0       Transfer Direction    (0=To Main RAM, 1=From Main RAM)
//  1       Memory Address Step   (0=Forward;+4, 1=Backward;-4)
//  2-7     Not used              (always zero)
//  8       Chopping Enable       (0=Normal, 1=Chopping; run CPU during DMA gaps)
//  9-10    SyncMode, Transfer Synchronisation/Mode (0-3):
//            0  Start immediately and transfer all at once (used for CDROM, OTC)
//            1  Sync blocks to DMA requests   (used for MDEC, SPU, and GPU-data)
//            2  Linked-List mode              (used for GPU-command-lists)
//            3  Reserved                      (not used)
//  11-15   Not used              (always zero)
//  16-18   Chopping DMA Window Size (1 SHL N words)
//  19      Not used              (always zero)
//  20-22   Chopping CPU Window Size (1 SHL N clks)
//  23      Not used              (always zero)
//  24      Start/Busy            (0=Stopped/Completed, 1=Start/Enable/Busy)
//  25-27   Not used              (always zero)
//  28      Start/Trigger         (0=Normal, 1=Manual Start; use for SyncMode=0)
//  29      Unknown (R/W) Pause?  (0=No, 1=Pause?)     (For SyncMode=0 only?)
//  30      Unknown (R/W)
//  31      Not used              (always zero)

#define DX_CHCR_START                   (UINT32_C(1) << 28)
#define DX_CHCR_BUSY                    (UINT32_C(1) << 24)
#define DX_CHCR_CHOPPING_ENABLE         (UINT32_C(1) << 8)
#define DX_CHCR_BACKWARD                (UINT32_C(1) << 1)
#define DX_CHCR_DIRECTION               (UINT32_C(1) << 0)

void write_dx_madr(int channel, uint32_t val) {
    debugger::info(Debugger::DMA, "d{}_madr <- {:08x}", channel, val);
    state.hw.dma[channel].madr = val & UINT32_C(0x00ffffff);
}

void write_dx_bcr(int channel, uint32_t val) {
    debugger::info(Debugger::DMA, "d{}_bcr <- {:08x}", channel, val);
    state.hw.dma[channel].bcr = val;
}

void write_d2_chcr(uint32_t val) {
    debugger::info(Debugger::DMA, "d2_chcr <- {:08x}", val);
    state.hw.dma[2].chcr = val;

    bool started = (val & DX_CHCR_BUSY) != 0;
    bool master_enabled = (state.hw.dpcr >> 11) & UINT32_C(1);

    if (!(started && master_enabled)) {
        return;
    }

    uint32_t bcr = state.hw.dma[2].bcr;
    uint32_t chcr = state.hw.dma[2].chcr;
    uint32_t addr = state.hw.dma[2].madr & UINT32_C(0xfffffc);
    bool from_ram = (chcr & DX_CHCR_DIRECTION) != 0;
    uint32_t sync_mode = (chcr >> 9) & UINT32_C(0x3);

    debugger::debug(Debugger::DMA, "GPU DMA started");
    debugger::debug(Debugger::DMA, "  address: {:08x}", addr);
    debugger::debug(Debugger::DMA, "  sync_mode: {}", sync_mode);

    if (sync_mode == 1) {
        // Block mode.
        // The BCR register holds the number of blocks:
        //   0-15  BS    Blocksize (words) ;for GPU/SPU max 10h, for MDEC max 20h
        //   16-31 BA    Amount of blocks  ;ie. total length = BS*BA words
        uint32_t block_size = (bcr >> 0) & UINT32_C(0xffff);
        uint32_t block_count = (bcr >> 16) & UINT32_C(0xffff);
        uint32_t total_len = 4 * block_size * block_count;

        if (block_size > 16) {
            psx::halt("invalid block size");
        }

        debugger::debug(Debugger::DMA, "  block_size: {}", block_size);
        debugger::debug(Debugger::DMA, "  block_count: {}", block_count);
        debugger::debug(Debugger::DMA, "  total_len: {}", total_len);

        if ((addr + total_len) > UINT32_C(0x200000)) {
            psx::halt("invalid block address");
            return;
        }

        for (uint32_t offset = 0; offset < total_len; offset += 4) {
            if (from_ram) {
                hw::write_gpu0(psx::memory::load_u32_le(
                    state.ram + addr + offset));
            }
            else {
                uint32_t val;
                hw::read_gpuread(&val);
                memory::store_u32_le(state.ram + addr + offset, val);
            }
        }

        // Address is updated after block transfer.
        state.hw.dma[2].madr = addr + total_len;
    }
    else if (sync_mode == 2) {
        // Linked-List mode.
        // Transfer commands as a linked list composed of words entries:
        //   item[23:0]    pointer to next item
        //   item[31:24]   item payload size in words
        //
        // Expected for transfer of Ordering Tables from CPU to GPU0.
        // DMA direction should be from RAM to GPU0.

        if (!from_ram || state.gpu.dma_direction != UINT8_C(2)) {
            psx::halt("unsupported GPU DMA direction in sync mode 0");
            return;
        }

        while (addr != UINT32_C(0xffffff)) {
            if (addr >= UINT32_C(0x200000) || (addr & UINT32_C(0x3)) != 0) {
                psx::halt("invalid OT address");
                return;
            }

            uint32_t entry = memory::load_u32_le(state.ram + addr);
            uint32_t nr_words = (entry >> 24) & UINT32_C(0xff);

            for (unsigned nr = 0; nr < nr_words; nr++) {
                uint32_t val = psx::memory::load_u32_le(
                    state.ram + addr + 4 + nr * 4);
                hw::write_gpu0(val);
            }

            addr = entry & UINT32_C(0xffffff);
        }
    }
    else {
        psx::halt("unsupported GPU DMA sync mode");
    }

    // Clear busy and start bits in CHCR register, set DMA flag.
    state.hw.dma[2].chcr &= ~(DX_CHCR_START | DX_CHCR_BUSY);
    if ((state.hw.dicr & DICR_IRQ_ENABLE(2)) != 0) {
        state.hw.dicr |= DICR_IRQ_FLAG(2);
        check_dicr_irq_master_flag();
    }
}

void write_d6_chcr(uint32_t val) {
    debugger::info(Debugger::DMA, "d6_chcr <- {:08x}", val);
    state.hw.dma[6].chcr = (val & UINT32_C(0x51000000)) | UINT32_C(0x2);

    bool started = (val & DX_CHCR_START) != 0;
    bool master_enabled = (state.hw.dpcr >> 27) & UINT32_C(1);

    if (!(started && master_enabled)) {
        return;
    }

    // Direction is always from GPU to RAM;
    // step is always backward,
    // sync mode is always 0.
    uint32_t bcr = state.hw.dma[6].bcr;
    uint32_t nr_words = bcr & UINT32_C(0xffff);
    uint32_t start_addr = state.hw.dma[6].madr;
    uint32_t end_addr = start_addr - (nr_words * 4);

    debugger::debug(Debugger::DMA, "OTC DMA started");
    debugger::debug(Debugger::DMA, "  address: {:08x}", start_addr);
    debugger::debug(Debugger::DMA, "  nr_words: {}", nr_words);

    // Initializes a linked list composed of words entries:
    //   item[23:0]    pointer to next item
    //   item[31:24]   item payload size in words
    // First check the start address, and that it fits in RAM memory.
    if (start_addr >= UINT32_C(0x200000) || end_addr > start_addr ||
        (start_addr & UINT32_C(0x3)) != 0) {
        psx::halt("invalid OTC address");
    }

    // Build the linked list.
    if (nr_words > 0) {
        memory::store_u32_le(state.ram + start_addr, UINT32_C(0x00ffffff));
        start_addr -= 4;
        for (unsigned nr = 1; nr < nr_words; nr++, start_addr -= 4) {
            memory::store_u32_le(
                state.ram + start_addr,
                (start_addr + 4) & UINT32_C(0x00ffffff));
        }
    }

    // Clear busy and start bits in CHCR register, set DMA flag.
    state.hw.dma[6].chcr &= ~(DX_CHCR_START | DX_CHCR_BUSY);
    if ((state.hw.dicr & DICR_IRQ_ENABLE(6)) != 0) {
        state.hw.dicr |= DICR_IRQ_FLAG(6);
        check_dicr_irq_master_flag();
    }
}

void write_dx_chcr(int channel, uint32_t val) {
    debugger::info(Debugger::DMA, "d{}_chcr <- {:08x}", channel, val);
    state.hw.dma[channel].chcr = val;

    bool started = (val & DX_CHCR_BUSY) != 0;
    bool master_enabled = (state.hw.dpcr >> (channel * 4 + 3)) & UINT32_C(1);

    if (!(started && master_enabled)) {
        return;
    }

    uint32_t madr = state.hw.dma[channel].madr;
    uint32_t chcr = state.hw.dma[channel].chcr;
    bool from_ram = (chcr & DX_CHCR_DIRECTION) != 0;
    uint32_t sync_mode = (chcr >> 9) & UINT32_C(0x3);

    debugger::debug(Debugger::DMA, "#{} DMA started", channel);
    debugger::debug(Debugger::DMA, "  sync_mode: {}", sync_mode);
    debugger::debug(Debugger::DMA, "  direction: {}",
        from_ram ? "from RAM" : "to RAM");
    debugger::debug(Debugger::DMA, "  address:   {:08x}", madr);

    psx::halt(fmt::format("DMA started for channel {}", channel));
}

};  // psx::hw
