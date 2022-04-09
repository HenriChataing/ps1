
#ifndef _HW_H_INCLUDED_
#define _HW_H_INCLUDED_

#include <cstdint>
#include <lib/types.h>

namespace psx::hw {

void read_joy_data(uint32_t *val);
void write_joy_data(uint32_t val);
void read_joy_stat(uint32_t *val);
void read_joy_ctrl(uint32_t *val);
void write_joy_ctrl(uint16_t val);
void read_joy_mode(uint32_t *val);
void write_joy_mode(uint16_t val);
void read_joy_baud(uint32_t *val);
void write_joy_baud(uint16_t val);

#define I_STAT_SPU      (UINT32_C(1) << 9)
#define I_STAT_SIO      (UINT32_C(1) << 8)
#define I_STAT_CTRL     (UINT32_C(1) << 7)
#define I_STAT_TMR2     (UINT32_C(1) << 6)
#define I_STAT_TMR1     (UINT32_C(1) << 5)
#define I_STAT_TMR0     (UINT32_C(1) << 4)
#define I_STAT_DMA      (UINT32_C(1) << 3)
#define I_STAT_CDROM    (UINT32_C(1) << 2)
#define I_STAT_GPU      (UINT32_C(1) << 1)
#define I_STAT_VBLANK   (UINT32_C(1) << 0)

void set_i_stat(uint32_t irq);

void read_i_stat(uint32_t *val);
void read_i_mask(uint32_t *val);
void write_i_stat(uint32_t val);
void write_i_mask(uint32_t val);

void read_timer_value(int timer, uint32_t *val);
void write_timer_value(int timer, uint16_t val);
void write_timer_mode(int timer, uint16_t val);
void write_timer_target(int timer, uint16_t val);

void read_dx_madr(int channel, uint32_t *val);
void write_dx_madr(int channel, uint32_t val);
void write_dx_bcr(int channel, uint32_t val);
void read_dx_chcr(int channel, uint32_t *val);
void write_d2_chcr(uint32_t val);
void write_d6_chcr(uint32_t val);
void write_dx_chcr(int channel, uint32_t val);
void read_dpcr(uint32_t *val);
void write_dpcr(uint32_t val);
void read_dicr(uint32_t *val);
void write_dicr(uint32_t val);

void read_cdrom_index(uint32_t *val);
void write_cdrom_index(uint8_t val);
void read_cdrom_reg01(uint32_t *val);
void write_cdrom_reg01(uint8_t val);
void read_cdrom_reg03(uint32_t *val);
void write_cdrom_reg03(uint8_t val);
void write_cdrom_reg02(uint8_t val);

void read_gpuread(uint32_t *val);
void read_gpustat(uint32_t *val);
void write_gpu0(uint32_t val);
void write_gpu1(uint32_t val);
void hblank_event();

void *generate_display(size_t *out_buffer_width, size_t *out_buffer_height,
                       size_t *out_display_width, size_t *out_display_height);

void *generate_display_vram_16bit(size_t *out_buffer_width,
                                  size_t *out_buffer_height,
                                  size_t *out_display_width,
                                  size_t *out_display_height);

}; /* namespace psx::hw */

#endif /* _HW_H_INCLUDED_ */
