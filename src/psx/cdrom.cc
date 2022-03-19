
#include <psx/psx.h>
#include <psx/hw.h>
#include <psx/debugger.h>
#include <psx/memory.h>

using namespace psx;

namespace psx::hw {

#define ADPBUSY     (UINT8_C(1) << 2)
#define PRMEMPT     (UINT8_C(1) << 3)
#define PRMWRDY     (UINT8_C(1) << 4)
#define RSLRRDY     (UINT8_C(1) << 5)
#define DRQSTS      (UINT8_C(1) << 6)
#define BUSYSTS     (UINT8_C(1) << 7)

#define INT1        (UINT8_C(1))
#define INT2        (UINT8_C(2))
#define INT3        (UINT8_C(3))
#define INT4        (UINT8_C(4))
#define INT5        (UINT8_C(5))

//  7  Play          Playing CD-DA         ;\only ONE of these bits can be set
//  6  Seek          Seeking               ; at a time (ie. Read/Play won't get
//  5  Read          Reading data sectors  ;/set until after Seek completion)
//  4  ShellOpen     Once shell open (0=Closed, 1=Is/was Open)
//  3  IdError       (0=Okay, 1=GetID denied) (also set when Setmode.Bit4=1)
//  2  SeekError     (0=Okay, 1=Seek error)     (followed by Error Byte)
//  1  Spindle Motor (0=Motor off, or in spin-up phase, 1=Motor on)
//  0  Error         Invalid Command/parameters (followed by Error Byte)

#define STAT_PLAY           (UINT8_C(1) << 7)
#define STAT_SEEK           (UINT8_C(1) << 6)
#define STAT_READ           (UINT8_C(1) << 5)
#define STAT_SHELL_OPEN     (UINT8_C(1) << 4)
#define STAT_ID_ERROR       (UINT8_C(1) << 3)
#define STAT_SEEK_ERROR     (UINT8_C(1) << 2)
#define STAT_SPINDLE_ERROR  (UINT8_C(1) << 1)
#define STAT_ERROR          (UINT8_C(1) << 0)

static uint8_t sync(uint8_t cmd) {
    psx::halt("sync not implemented");
    return INT5;
}

static uint8_t get_stat(uint8_t cmd) {
    state.cdrom.response_fifo[0] = state.cdrom.stat;
    state.cdrom.response_fifo_length = 1;
    state.cdrom.stat &= ~STAT_SHELL_OPEN;
    return INT3;
}

static uint8_t set_loc(uint8_t cmd) {
    psx::halt("set_loc not implemented");
    return INT5;
}

static uint8_t play(uint8_t cmd) {
    psx::halt("play not implemented");
    return INT5;
}

static uint8_t forward(uint8_t cmd) {
    psx::halt("forward not implemented");
    return INT5;
}

static uint8_t backward(uint8_t cmd) {
    psx::halt("backward not implemented");
    return INT5;
}

static uint8_t read_N(uint8_t cmd) {
    psx::halt("read_N not implemented");
    return INT5;
}

static uint8_t motor_on(uint8_t cmd) {
    psx::halt("motor_on not implemented");
    return INT5;
}

static uint8_t stop(uint8_t cmd) {
    psx::halt("stop not implemented");
    return INT5;
}

static uint8_t pause(uint8_t cmd) {
    psx::halt("pause not implemented");
    return INT5;
}

static uint8_t init(uint8_t cmd) {
    psx::halt("init not implemented");
    return INT5;
}

static uint8_t mute(uint8_t cmd) {
    psx::halt("mute not implemented");
    return INT5;
}

static uint8_t demute(uint8_t cmd) {
    psx::halt("demute not implemented");
    return INT5;
}

static uint8_t set_filter(uint8_t cmd) {
    psx::halt("set_filter not implemented");
    return INT5;
}

static uint8_t set_mode(uint8_t cmd) {
    psx::halt("set_mode not implemented");
    return INT5;
}

static uint8_t get_param(uint8_t cmd) {
    psx::halt("get_param not implemented");
    return INT5;
}

static uint8_t get_loc_L(uint8_t cmd) {
    psx::halt("get_loc_L not implemented");
    return INT5;
}

static uint8_t get_loc_P(uint8_t cmd) {
    psx::halt("get_loc_P not implemented");
    return INT5;
}

static uint8_t set_session(uint8_t cmd) {
    psx::halt("set_session not implemented");
    return INT5;
}

static uint8_t get_TN(uint8_t cmd) {
    psx::halt("get_TN not implemented");
    return INT5;
}

static uint8_t get_TD(uint8_t cmd) {
    psx::halt("get_TD not implemented");
    return INT5;
}

static uint8_t seek_L(uint8_t cmd) {
    psx::halt("seek_L not implemented");
    return INT5;
}

static uint8_t seek_P(uint8_t cmd) {
    psx::halt("seek_P not implemented");
    return INT5;
}

static uint8_t set_clock(uint8_t cmd) {
    psx::halt("set_clock not implemented");
    return INT5;
}

static uint8_t get_clock(uint8_t cmd) {
    psx::halt("get_clock not implemented");
    return INT5;
}

#define CTRVER_vC0_a    UINT32_C(0x940919C0)  // PSX (PU-7)               19 Sep 1994, version vC0 (a)
#define CTRVER_vC0_b    UINT32_C(0x941118C0)  // PSX (PU-7)               18 Nov 1994, version vC0 (b)
#define CTRVER_vC1_a    UINT32_C(0x950516C1)  // PSX (LATE-PU-8)          16 May 1995, version vC1 (a)
#define CTRVER_vC1_b    UINT32_C(0x950724C1)  // PSX (LATE-PU-8)          24 Jul 1995, version vC1 (b)
#define CTRVER_vD1_debug    UINT32_C(0x950724D1)  // PSX (LATE-PU-8,debug ver)24 Jul 1995, version vD1 (debug)
#define CTRVER_vC2_VCD      UINT32_C(0x960815C2)  // PSX (PU-16, Video CD)    15 Aug 1996, version vC2 (VCD)
#define CTRVER_vC1_yaroze   UINT32_C(0x960818C1)  // PSX (LATE-PU-8,yaroze)   18 Aug 1996, version vC1 (yaroze)
#define CTRVER_vC2_a_jap    UINT32_C(0x960912C2)  // PSX (PU-18) (japan)      12 Sep 1996, version vC2 (a.jap)
#define CTRVER_vC2_a    UINT32_C(0x970110C2)  // PSX (PU-18) (us/eur)     10 Jan 1997, version vC2 (a)
#define CTRVER_vC2_b    UINT32_C(0x970814C2)  // PSX (PU-20)              14 Aug 1997, version vC2 (b)
#define CTRVER_vC3_a    UINT32_C(0x980610C3)  // PSX (PU-22)              10 Jul 1998, version vC3 (a)
#define CTRVER_vC3_b    UINT32_C(0x990201C3)  // PSX/PSone (PU-23, PM-41) 01 Feb 1999, version vC3 (b)
#define CTRVER_vC3_c    UINT32_C(0xA10306C3)  // PSone/late (PM-41(2))    06 Jun 2001, version vC3 (c)

static uint8_t test(uint8_t cmd) {
    if (state.cdrom.parameter_fifo_index != 1) {
        psx::halt("missing test parameters");
    }

    uint8_t sub_function = state.cdrom.parameter_fifo[0];
    switch (sub_function) {
    case UINT8_C(0x20): {
        uint32_t version = CTRVER_vC3_a;
        state.cdrom.response_fifo[0] = version >> 24; // yy
        state.cdrom.response_fifo[1] = version >> 16; // mm
        state.cdrom.response_fifo[2] = version >> 8;  // dd
        state.cdrom.response_fifo[3] = version >> 0;  // version
        state.cdrom.response_fifo_length = 4;
        return INT3;
    }

    default:
        psx::halt(fmt::format(
            "test sub_function {:x} not supported", sub_function));
        return INT5;
    }
}

static uint8_t get_ID(uint8_t cmd) {
    psx::halt("get_ID not implemented");
    return INT5;
}

static uint8_t read_S(uint8_t cmd) {
    psx::halt("read_S not implemented");
    return INT5;
}

static uint8_t reset(uint8_t cmd) {
    psx::halt("reset not implemented");
    return INT5;
}

static uint8_t get_Q(uint8_t cmd) {
    psx::halt("get_Q not implemented");
    return INT5;
}

static uint8_t read_TOC(uint8_t cmd) {
    psx::halt("read_TOC not implemented");
    return INT5;
}

static uint8_t video_CD(uint8_t cmd) {
    psx::halt("video_CD not implemented");
    return INT5;
}

static struct {
    char const *name;
    uint8_t (*handler)(uint8_t cmd);
} cdrom_commands[32] = {
    { "Sync", sync },
    { "Getstat", get_stat },
    { "Setloc", set_loc },
    { "Play", play },
    { "Forward", forward },
    { "Backward", backward },
    { "ReadN", read_N },
    { "MotorOn", motor_on },
    { "Stop", stop },
    { "Pause", pause },
    { "Init", init },
    { "Mute", mute },
    { "Demute", demute },
    { "Setfilter", set_filter },
    { "Setmode", set_mode },
    { "Getparam", get_param },
    { "GetlocL", get_loc_L },
    { "GetlocP", get_loc_P },
    { "SetSession", set_session },
    { "GetTN", get_TN },
    { "GetTD", get_TD },
    { "SeekL", seek_L },
    { "SeekP", seek_P },
    { "SetClock?", set_clock },
    { "GetClock?", get_clock },
    { "Test", test },
    { "GetID", get_ID },
    { "ReadS", read_S },
    { "Reset", reset },
    { "GetQ", get_Q },
    { "ReadTOC", read_TOC },
    { "VideoCD", video_CD },
};

static void cdrom_command(uint8_t cmd) {
    uint8_t sig;
    if (cmd < 0x20) {
        debugger::info(Debugger::CDROM, "{}", cdrom_commands[cmd].name);
        sig = cdrom_commands[cmd].handler(cmd);
    } else {
        debugger::info(Debugger::CDROM, "unknown command 0x{:x}", cmd);
        sig = INT5;
    }

    state.cdrom.response_fifo_index = 0;
    state.cdrom.parameter_fifo_index = 0;
    state.cdrom.interrupt_flag |= sig;

    if (state.cdrom.response_fifo_length > 0) {
        state.cdrom.index |= RSLRRDY;
    } else {
        state.cdrom.index &= ~RSLRRDY;
    }

    if (state.cdrom.interrupt_flag & state.cdrom.interrupt_enable) {
        hw::set_i_stat(I_STAT_CDROM);
    }
}

//  0-1 Index   Port 1F801801h-1F801803h index (0..3 = Index0..Index3)   (R/W)
//  2   ADPBUSY XA-ADPCM fifo empty  (0=Empty) ;set when playing XA-ADPCM sound
//  3   PRMEMPT Parameter fifo empty (1=Empty) ;triggered before writing 1st byte
//  4   PRMWRDY Parameter fifo full  (0=Full)  ;triggered after writing 16 bytes
//  5   RSLRRDY Response fifo empty  (0=Empty) ;triggered after reading LAST byte
//  6   DRQSTS  Data fifo empty      (0=Empty) ;triggered after reading LAST byte
//  7   BUSYSTS Command/parameter transmission busy  (1=Busy)

void read_cdrom_index(uint32_t *val) {
    debugger::info(Debugger::CDROM, "cdrom_index -> {:02x}", state.cdrom.index);
    *val = state.cdrom.index;
}

void write_cdrom_index(uint8_t val) {
    debugger::info(Debugger::CDROM, "cdrom_index <- {:02x}", val);
    state.cdrom.index &= ~UINT8_C(0x3);
    state.cdrom.index |= val & UINT8_C(0x3);
}

void read_cdrom_reg01(uint32_t *val) {
    *val = state.cdrom.response_fifo[state.cdrom.response_fifo_index];
    debugger::info(Debugger::CDROM, "cdrom_response -> {:02x}", *val);

    state.cdrom.response_fifo_index++;
    if (state.cdrom.response_fifo_index >= state.cdrom.response_fifo_length) {
        state.cdrom.index &= ~RSLRRDY;
    }
    if (state.cdrom.response_fifo_index > sizeof(state.cdrom.response_fifo)) {
        state.cdrom.response_fifo_index = 0;
    }
}

void write_cdrom_reg01(uint8_t val) {
    unsigned index = state.cdrom.index & UINT8_C(0x3);
    switch (index) {
    case 0x0:
        debugger::info(Debugger::CDROM, "cdrom_command <- {:02x}", val);
        state.cdrom.command = val;
        cdrom_command(val);
        break;

    default:
        debugger::info(Debugger::CDROM, "cdrom reg01.index{} <- {:02x}", index, val);
        psx::halt("CDROM unsupported write");
        break;
    }
}

void write_cdrom_reg02(uint8_t val) {
    unsigned index = state.cdrom.index & UINT8_C(0x3);
    unsigned fifo_index;

    switch (index) {
    case 0x0:
        debugger::info(Debugger::CDROM, "cdrom_parameter_fifo <- {:02x}", val);
        fifo_index = state.cdrom.parameter_fifo_index;
        if (fifo_index >= 15) {
            psx::halt("CDROM parameter fifo overflow");
            return;
        }
        state.cdrom.parameter_fifo[fifo_index] = val;
        state.cdrom.parameter_fifo_index = fifo_index + 1;
        state.cdrom.index &= ~PRMEMPT;
        if (fifo_index + 1 >= 15) {
            state.cdrom.index &= ~PRMWRDY;
        }
        break;

    case 0x1:
        debugger::info(Debugger::CDROM, "cdrom_interrupt_enable <- {:02x}", val);
        state.cdrom.interrupt_enable = val & UINT8_C(0x1f);
        break;

    default:
        debugger::info(Debugger::CDROM, "cdrom reg02.index{} <- {:02x}", index, val);
        psx::halt("CDROM unsupported write");
        break;
    }
}

void read_cdrom_reg03(uint32_t *val) {
    unsigned index = state.cdrom.index & UINT8_C(0x3);
    switch (index) {
    case 0x0:
    case 0x2:
        *val = state.cdrom.interrupt_enable;
        debugger::info(Debugger::CDROM, "cdrom_interrupt_enable -> {:02x}", *val);
        break;

    case 0x1:
    case 0x3:
        *val = state.cdrom.interrupt_flag | UINT8_C(0xe0);
        debugger::info(Debugger::CDROM, "cdrom_interrupt_flag -> {:02x}", *val);
        break;

    default:
        *val = 0;
        debugger::warn(Debugger::CDROM, "cdrom reg03.index{} -> ?", index);
        psx::halt("CDROM unsupported read");
        break;
    }
}

void write_cdrom_reg03(uint8_t val) {
    unsigned index = state.cdrom.index & UINT8_C(0x3);
    switch (index) {
    case 0x0:
        debugger::info(Debugger::CDROM, "cdrom_request <- {:02x}", val);
        state.cdrom.request = val;
        break;

    case 0x1:
    case 0x3:
        debugger::info(Debugger::CDROM, "cdrom_interrupt_flag <- {:02x}", val);
        state.cdrom.interrupt_flag &= ~(val & UINT8_C(0x1f));
        break;

    default:
        debugger::info(Debugger::CDROM, "cdrom reg03.index{} <- {:02x}", index, val);
        psx::halt("CDROM unsupported write");
        break;
    }
}


};  // psx::hw
