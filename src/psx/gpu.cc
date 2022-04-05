
#include <cassert>
#include <algorithm>
#include <psx/psx.h>
#include <psx/hw.h>
#include <psx/debugger.h>
#include <gui/graphics.h>

using namespace psx;

namespace psx::hw {

//  0-3   Texture page X Base   (N*64)                              ;GP0(E1h).0-3
//  4     Texture page Y Base   (N*256) (ie. 0 or 256)              ;GP0(E1h).4
//  5-6   Semi Transparency     (0=B/2+F/2, 1=B+F, 2=B-F, 3=B+F/4)  ;GP0(E1h).5-6
//  7-8   Texture page colors   (0=4bit, 1=8bit, 2=15bit, 3=Reserved)GP0(E1h).7-8
//  9     Dither 24bit to 15bit (0=Off/strip LSBs, 1=Dither Enabled);GP0(E1h).9
//  10    Drawing to display area (0=Prohibited, 1=Allowed)         ;GP0(E1h).10
//  11    Set Mask-bit when drawing pixels (0=No, 1=Yes/Mask)       ;GP0(E6h).0
//  12    Draw Pixels           (0=Always, 1=Not to Masked areas)   ;GP0(E6h).1
//  13    Interlace Field       (or, always 1 when GP1(08h).5=0)
//  14    "Reverseflag"         (0=Normal, 1=Distorted)             ;GP1(08h).7
//  15    Texture Disable       (0=Normal, 1=Disable Textures)      ;GP0(E1h).11
//  16    Horizontal Resolution 2     (0=256/320/512/640, 1=368)    ;GP1(08h).6
//  17-18 Horizontal Resolution 1     (0=256, 1=320, 2=512, 3=640)  ;GP1(08h).0-1
//  19    Vertical Resolution         (0=240, 1=480, when Bit22=1)  ;GP1(08h).2
//  20    Video Mode                  (0=NTSC/60Hz, 1=PAL/50Hz)     ;GP1(08h).3
//  21    Display Area Color Depth    (0=15bit, 1=24bit)            ;GP1(08h).4
//  22    Vertical Interlace          (0=Off, 1=On)                 ;GP1(08h).5
//  23    Display Enable              (0=Enabled, 1=Disabled)       ;GP1(03h).0
//  24    Interrupt Request (IRQ1)    (0=Off, 1=IRQ)       ;GP0(1Fh)/GP1(02h)
//  25    DMA / Data Request, meaning depends on GP1(04h) DMA Direction:
//          When GP1(04h)=0 ---> Always zero (0)
//          When GP1(04h)=1 ---> FIFO State  (0=Full, 1=Not Full)
//          When GP1(04h)=2 ---> Same as GPUSTAT.28
//          When GP1(04h)=3 ---> Same as GPUSTAT.27
//  26    Ready to receive Cmd Word   (0=No, 1=Ready)  ;GP0(...) ;via GP0
//  27    Ready to send VRAM to CPU   (0=No, 1=Ready)  ;GP0(C0h) ;via GPUREAD
//  28    Ready to receive DMA Block  (0=No, 1=Ready)  ;GP0(...) ;via GP0
//  29-30 DMA Direction (0=Off, 1=?, 2=CPUtoGP0, 3=GPUREADtoCPU)    ;GP1(04h).0-1
//  31    Drawing even/odd lines in interlace mode (0=Even or Vblank, 1=Odd)

#define GPUSTAT_VBLANK                 (UINT32_C(1) << 31)
#define GPUSTAT_DMA_READY              (UINT32_C(1) << 28)
#define GPUSTAT_COPY_READY             (UINT32_C(1) << 27)
#define GPUSTAT_CMD_READY              (UINT32_C(1) << 26)
#define GPUSTAT_DMA                    (UINT32_C(1) << 25)
#define GPUSTAT_INT                    (UINT32_C(1) << 24)
#define GPUSTAT_DISPLAY_DISABLE        (UINT32_C(1) << 23)
#define GPUSTAT_VERTICAL_INTERLACE     (UINT32_C(1) << 22)
#define GPUSTAT_COLOR_DEPTH            (UINT32_C(1) << 21)
#define GPUSTAT_VIDEO_MODE             (UINT32_C(1) << 20)
#define GPUSTAT_VERTICAL_RESOLUTION    (UINT32_C(1) << 19)
#define GPUSTAT_TEXTURE_DISABLE        (UINT32_C(1) << 15)
#define GPUSTAT_REVERSE_FLAG           (UINT32_C(1) << 14)
#define GPUSTAT_INTERLACE_FIELD        (UINT32_C(1) << 13)

/// Re-evaluate the location of the displayed framebuffer given
/// the current video configuration.
void *generate_display(size_t *out_buffer_width, size_t *out_buffer_height,
                       size_t *out_display_width, size_t *out_display_height) {
    if (!state.gpu.display_enable) {
        return NULL;
    }
    if ((state.gpu.vertical_display_range_y2 <
         state.gpu.vertical_display_range_y1) ||
        (state.gpu.horizontal_display_range_x2 <
         state.gpu.horizontal_display_range_x1)) {
        return NULL;
    }

    uint8_t const *framebuffer_address = state.vram +
        state.gpu.start_of_display_area_y * 2048 +
        state.gpu.start_of_display_area_x * 2;

    size_t color_depth = state.gpu.display_area_color_depth ? 24 : 16;
    size_t framebuffer_height = state.gpu.vertical_resolution ? 480 : 240;
    size_t width;

    switch (state.gpu.horizontal_resolution) {
    case 0x0: width = 256; break;
    case 0x1: width = 320; break;
    case 0x2: width = 512; break;
    case 0x3: width = 640; break;
    default:  width = 368; break;
    }

    size_t display_height =
        state.gpu.vertical_display_range_y2 -
        state.gpu.vertical_display_range_y1;
    if (state.gpu.vertical_interlace) {
        framebuffer_height /= 2;
    }

    size_t height = std::min(framebuffer_height, display_height);
    unsigned char *framebuffer = (unsigned char *)calloc(width * height, 3);
    unsigned char *src_data = (unsigned char *)framebuffer_address;
    unsigned char *dst_data = framebuffer;

    if (color_depth == 24 && state.gpu.vertical_interlace) {
        for (unsigned y = 0; y < height; y++, src_data += 4096) {
            for (unsigned x = 0; x < width; x++, dst_data += 3) {
                unsigned char r0 = src_data[3 * x + 0];
                unsigned char g0 = src_data[3 * x + 1];
                unsigned char b0 = src_data[3 * x + 2];
                unsigned char r1 = src_data[3 * x + 0 + 2048];
                unsigned char g1 = src_data[3 * x + 1 + 2048];
                unsigned char b1 = src_data[3 * x + 2 + 2048];
                dst_data[0] = (r0 + r1) / 2;
                dst_data[1] = (g0 + g1) / 2;
                dst_data[2] = (b0 + b1) / 2;
            }
        }
    } else if (color_depth == 24) {
        for (unsigned y = 0; y < height; y++, src_data += 2048) {
            memcpy(dst_data, src_data, width * 3);
            dst_data += width * 3;

        }
    } else if (state.gpu.vertical_interlace) {
        for (unsigned y = 0; y < height; y++, src_data += 4096) {
            for (unsigned x = 0; x < width; x++, dst_data += 3) {
                uint16_t rgb0 =
                    ((uint16_t)(src_data[2 * x + 0]) << 0) |
                    ((uint16_t)(src_data[2 * x + 1]) << 8);
                uint16_t rgb1 =
                    ((uint16_t)(src_data[2 * x + 0 + 2048]) << 0) |
                    ((uint16_t)(src_data[2 * x + 1 + 2048]) << 8);

                unsigned char r0 = ((rgb0 >>  0) & 0x1f) << 3;
                unsigned char g0 = ((rgb0 >>  5) & 0x1f) << 3;
                unsigned char b0 = ((rgb0 >> 10) & 0x1f) << 3;
                unsigned char r1 = ((rgb1 >>  0) & 0x1f) << 3;
                unsigned char g1 = ((rgb1 >>  5) & 0x1f) << 3;
                unsigned char b1 = ((rgb1 >> 10) & 0x1f) << 3;
                dst_data[0] = (r0 + r1) / 2;
                dst_data[1] = (g0 + g1) / 2;
                dst_data[2] = (b0 + b1) / 2;
            }
        }
    } else {
        for (unsigned y = 0; y < height; y++, src_data += 2048) {
            for (unsigned x = 0; x < width; x++, dst_data += 3) {
                uint16_t rgb =
                    ((uint16_t)(src_data[2 * x + 0]) << 0) |
                    ((uint16_t)(src_data[2 * x + 1]) << 8);
                dst_data[0] = ((rgb >>  0) & 0x1f) << 3;
                dst_data[1] = ((rgb >>  5) & 0x1f) << 3;
                dst_data[2] = ((rgb >> 10) & 0x1f) << 3;
            }
        }
    }

    *out_buffer_width = width;
    *out_buffer_height = height;
    *out_display_width = 320;
    *out_display_height = 240;
    return framebuffer;
}

void read_gpuread(uint32_t *val) {
    debugger::debug(Debugger::GPU, "gpuread -> {:08x}", *val);

    if (state.gp0.state != GP0_COPY_VRAM_TO_CPU) {
        *val = 0;
        return;
    }

    uint32_t x = (state.gp0.transfer.x0 + state.gp0.transfer.x) & UINT16_C(0x3ff);
    uint32_t y = (state.gp0.transfer.y0 + state.gp0.transfer.y) & UINT16_C(0x1ff);
    uint16_t lo;
    uint16_t hi;

    lo = memory::load_u16_le(state.vram + y * 2048 + 2 * x);
    state.gp0.transfer.x++;
    x++;
    if (state.gp0.transfer.x >= state.gp0.transfer.width) {
        state.gp0.transfer.x = 0;
        state.gp0.transfer.y++;
        x = state.gp0.transfer.x0;
        y++;
    }

    hi = memory::load_u16_le(state.vram + y * 2048 + 2 * x);
    state.gp0.transfer.x++;
    if (state.gp0.transfer.x >= state.gp0.transfer.width) {
        state.gp0.transfer.x = 0;
        state.gp0.transfer.y++;
    }

    if (state.gp0.transfer.y >= state.gp0.transfer.height) {
        debugger::info(Debugger::GPU, "VRAM to CPU transfer complete");
        state.gp0.state = GP0_COMMAND;
        state.gp0.count = 0;
        state.hw.gpustat |= GPUSTAT_CMD_READY;
        state.hw.gpustat |= GPUSTAT_DMA_READY;
        state.hw.gpustat &= ~GPUSTAT_COPY_READY;
    }

    *val = (uint32_t)lo | ((uint32_t)hi << 16);
}

void read_gpustat(uint32_t *val) {
    *val = state.hw.gpustat;
    debugger::debug(Debugger::GPU, "gpustat -> {:08x}", *val);
}

static int16_t sext_i11_i16(uint16_t val) {
    if (val & UINT16_C(0x0400)) {
        val |= UINT16_C(0xf800);
    }
    return (int16_t)val;
}

struct render_attributes {
    bool blended;
    bool semi_transparency;
    bool texture_mapping;
    bool gouraud_shading;
};

struct vertex_attributes {
    // Vertex coordinates
    int16_t x;
    int16_t y;
    // Color
    uint8_t r;
    uint8_t g;
    uint8_t b;
    // Texture coordinates
    uint8_t s;
    uint8_t t;
};

/// Render a single pixel.
/// Color depth is always 16bit in drawing area.
static void render_pixel(vertex_attributes pixel, render_attributes attributes)
{
    size_t pixel_width = 2;
    uint8_t *framebuffer_address = state.vram;
    uint8_t *pixel_address = framebuffer_address +
        pixel.y * 2048 +
        pixel.x * pixel_width;

    // TODO check drawing_to_display_area_enable

    uint16_t back_color = memory::load_u16_le(pixel_address);
    uint8_t back_r = ((back_color >> 0) & UINT16_C(0x1f)) << 3;
    uint8_t back_g = ((back_color >> 5) & UINT16_C(0x1f)) << 3;
    uint8_t back_b = ((back_color >> 10) & UINT16_C(0x1f)) << 3;
    uint8_t bit_mask = (back_color >> 15) & UINT16_C(0x1);

    if (state.gpu.check_bit_mask && bit_mask) {
        return;
    }

    bit_mask = 0;
    if (state.gpu.force_bit_mask) {
        bit_mask = 1;
    }

    if (attributes.semi_transparency) {
        switch (state.gpu.semi_transparency_mode) {
        case 0x0:
            pixel.r = (back_r / 2) + (pixel.r / 2);
            pixel.g = (back_g / 2) + (pixel.g / 2);
            pixel.b = (back_b / 2) + (pixel.b / 2);
            break;
        case 0x1:
            pixel.r = back_r + pixel.r;
            pixel.g = back_g + pixel.g;
            pixel.b = back_b + pixel.b;
            break;
        case 0x2:
            pixel.r = back_r - pixel.r;
            pixel.g = back_g - pixel.g;
            pixel.b = back_b - pixel.b;
            break;
        case 0x3:
        default:
            pixel.r = (back_r + pixel.r) / 4;
            pixel.g = (back_g + pixel.g) / 4;
            pixel.b = (back_b + pixel.b) / 4;
            break;
        }
    }

    if (state.gpu.dither_enable) {
        // TODO dither.
        pixel.r >>= 3;
        pixel.g >>= 3;
        pixel.b >>= 3;
    } else {
        pixel.r >>= 3;
        pixel.g >>= 3;
        pixel.b >>= 3;
    }

    uint16_t color =
        ((uint16_t)pixel.r << 0)  |
        ((uint16_t)pixel.g << 5)  |
        ((uint16_t)pixel.b << 10) |
        ((uint16_t)bit_mask << 15);
    memory::store_u16_le(pixel_address, color);
}


/// Edge function to place each point inside or outside a triangle.
static float edge_function(int16_t x0, int16_t y0,
                           int16_t x1, int16_t y1,
                           int16_t x2, int16_t y2) {
    return (float)(x2 - x0) * (float)(y1 - y0) -
           (float)(y2 - y0) * (float)(x1 - x0);
}

/// Rasterize and render a triangle.
static void render_triangle(vertex_attributes a,
                            vertex_attributes b,
                            vertex_attributes c,
                            render_attributes attributes) {

    a.x += state.gpu.drawing_offset_x;
    b.x += state.gpu.drawing_offset_x;
    c.x += state.gpu.drawing_offset_x;
    a.y += state.gpu.drawing_offset_y;
    b.y += state.gpu.drawing_offset_y;
    c.y += state.gpu.drawing_offset_y;

    int16_t x0 = std::max<int16_t>(0,    std::min({a.x, b.x, c.x, state.gpu.drawing_area_x1}));
    int16_t x1 = std::min<int16_t>(1024, std::max({a.x, b.x, c.x, state.gpu.drawing_area_x2}));
    int16_t y0 = std::max<int16_t>(0,    std::min({a.y, b.y, c.y, state.gpu.drawing_area_y1}));
    int16_t y1 = std::min<int16_t>(512,  std::max({a.y, b.y, c.y, state.gpu.drawing_area_y2}));

    // Compute edge function for Vc for Va, Vb.
    // - if == 0 the triangle is flat and not rendered.
    // - if < 0 the vertices are ordered in reverse order, swap
    //   Va, Vb to restore sign of edge function.
    float area = edge_function(a.x, a.y, b.x, b.y, c.x, c.y);
    if (area == 0.) {
        return;
    }
    if (area < 0.) {
        std::swap(a, b);
        area = -area;
    }

    for (int16_t y = y0; y <= y1; y++) {
        for (int16_t x = x0; x <= x1; x++) {
            float wa = edge_function(b.x, b.y, c.x, c.y, x, y);
            float wb = edge_function(c.x, c.y, a.x, a.y, x, y);
            float wc = edge_function(a.x, a.y, b.x, b.y, x, y);
            bool inside = wa >= 0. && wb >= 0. && wc >= 0.;
            if (inside) {
                vertex_attributes pixel;
                pixel.x = x;
                pixel.y = y;
                pixel.r = (a.r * wa + b.r * wb + c.r * wc) / area;
                pixel.g = (a.g * wa + b.g * wb + c.g * wc) / area;
                pixel.b = (a.b * wa + b.b * wb + c.b * wc) / area;
                render_pixel(pixel, attributes);
            }
        }
    }
}

static void nop(void) {
}

static void fill_rectangle(void) {
    uint8_t r = state.gp0.buffer[0];
    uint8_t g = state.gp0.buffer[0] >> 8;
    uint8_t b = state.gp0.buffer[0] >> 16;
    uint16_t x0 = state.gp0.buffer[1];
    uint16_t y0 = state.gp0.buffer[1] >> 16;
    uint16_t width  = state.gp0.buffer[2];
    uint16_t height = state.gp0.buffer[2] >> 16;

    render_attributes attributes = {
        .blended = false,
        .semi_transparency = false,
        .texture_mapping = false,
        .gouraud_shading = false,
    };

    for (uint16_t y = 0; y < height; y++) {
        for (uint16_t x = 0; x < width; x++) {
            vertex_attributes pixel;
            pixel.x = x0 + x;
            pixel.y = y0 + y;
            pixel.r = r;
            pixel.g = g;
            pixel.b = b;
            render_pixel(pixel, attributes);
        }
    }
}

static void monochrome_4p_polygon_opaque(void) {
    vertex_attributes va = { 0 };
    vertex_attributes vb = { 0 };
    vertex_attributes vc = { 0 };
    vertex_attributes vd = { 0 };

    uint8_t r = state.gp0.buffer[0];
    uint8_t g = state.gp0.buffer[0] >> 8;
    uint8_t b = state.gp0.buffer[0] >> 16;

    va.x = sext_i11_i16(state.gp0.buffer[1]);
    va.y = sext_i11_i16(state.gp0.buffer[1] >> 16);
    vb.x = sext_i11_i16(state.gp0.buffer[2]);
    vb.y = sext_i11_i16(state.gp0.buffer[2] >> 16);
    vc.x = sext_i11_i16(state.gp0.buffer[3]);
    vc.y = sext_i11_i16(state.gp0.buffer[3] >> 16);
    vd.x = sext_i11_i16(state.gp0.buffer[4]);
    vd.y = sext_i11_i16(state.gp0.buffer[4] >> 16);

    va.r = vb.r = vc.r = vd.r = r;
    va.g = vb.g = vc.g = vd.g = g;
    va.b = vb.b = vc.b = vd.b = b;

    render_attributes attributes = {
        .blended = false,
        .semi_transparency = false,
        .texture_mapping = false,
        .gouraud_shading = false,
    };

    render_triangle(va, vb, vc, attributes);
    render_triangle(vb, vc, vd, attributes);
}

static void shaded_3p_polygon_opaque(void) {
    vertex_attributes va = { 0 };
    vertex_attributes vb = { 0 };
    vertex_attributes vc = { 0 };

    va.r = state.gp0.buffer[0];
    va.g = state.gp0.buffer[0] >> 8;
    va.b = state.gp0.buffer[0] >> 16;
    va.x = sext_i11_i16(state.gp0.buffer[1]);
    va.y = sext_i11_i16(state.gp0.buffer[1] >> 16);

    vb.r = state.gp0.buffer[2];
    vb.g = state.gp0.buffer[2] >> 8;
    vb.b = state.gp0.buffer[2] >> 16;
    vb.x = sext_i11_i16(state.gp0.buffer[3]);
    vb.y = sext_i11_i16(state.gp0.buffer[3] >> 16);

    vc.r = state.gp0.buffer[4];
    vc.g = state.gp0.buffer[4] >> 8;
    vc.b = state.gp0.buffer[4] >> 16;
    vc.x = sext_i11_i16(state.gp0.buffer[5]);
    vc.y = sext_i11_i16(state.gp0.buffer[5] >> 16);

    render_attributes attributes = {
        .blended = false,
        .semi_transparency = false,
        .texture_mapping = false,
        .gouraud_shading = false,
    };

    render_triangle(va, vb, vc, attributes);
}

static void shaded_4p_polygon_opaque(void) {
    vertex_attributes va = { 0 };
    vertex_attributes vb = { 0 };
    vertex_attributes vc = { 0 };
    vertex_attributes vd = { 0 };

    va.r = state.gp0.buffer[0];
    va.g = state.gp0.buffer[0] >> 8;
    va.b = state.gp0.buffer[0] >> 16;
    va.x = sext_i11_i16(state.gp0.buffer[1]);
    va.y = sext_i11_i16(state.gp0.buffer[1] >> 16);

    vb.r = state.gp0.buffer[2];
    vb.g = state.gp0.buffer[2] >> 8;
    vb.b = state.gp0.buffer[2] >> 16;
    vb.x = sext_i11_i16(state.gp0.buffer[3]);
    vb.y = sext_i11_i16(state.gp0.buffer[3] >> 16);

    vc.r = state.gp0.buffer[4];
    vc.g = state.gp0.buffer[4] >> 8;
    vc.b = state.gp0.buffer[4] >> 16;
    vc.x = sext_i11_i16(state.gp0.buffer[5]);
    vc.y = sext_i11_i16(state.gp0.buffer[5] >> 16);

    vd.r = state.gp0.buffer[6];
    vd.g = state.gp0.buffer[6] >> 8;
    vd.b = state.gp0.buffer[6] >> 16;
    vd.x = sext_i11_i16(state.gp0.buffer[7]);
    vd.y = sext_i11_i16(state.gp0.buffer[7] >> 16);

    render_attributes attributes = {
        .blended = false,
        .semi_transparency = false,
        .texture_mapping = false,
        .gouraud_shading = false,
    };

    render_triangle(va, vb, vc, attributes);
    render_triangle(vb, vc, vd, attributes);
}

static void copy_rectangle_cpu_to_vram(void) {
    uint16_t x = (state.gp0.buffer[1] >> 0) & UINT32_C(0xffff);
    uint16_t y = (state.gp0.buffer[1] >> 16) & UINT32_C(0xffff);
    uint16_t width = (state.gp0.buffer[2] >> 0) & UINT32_C(0xffff);;
    uint16_t height = (state.gp0.buffer[2] >> 16) & UINT32_C(0xffff);;

    state.gp0.state = GP0_COPY_CPU_TO_VRAM;
    state.gp0.transfer.x0 = x & UINT16_C(0x3ff);
    state.gp0.transfer.y0 = y & UINT16_C(0x1ff);
    state.gp0.transfer.width = ((width - 1) & UINT16_C(0x3ff)) + 1;
    state.gp0.transfer.height = ((height - 1) & UINT16_C(0x1ff)) + 1;
    state.gp0.transfer.x = 0;
    state.gp0.transfer.y = 0;

    debugger::info(Debugger::GPU, "CPU to VRAM transfer size: {}",
                   2 * state.gp0.transfer.width * state.gp0.transfer.height);
    debugger::info(Debugger::GPU, "  x: {}", x);
    debugger::info(Debugger::GPU, "  y: {}", y);
    debugger::info(Debugger::GPU, "  width: {}", state.gp0.transfer.width);
    debugger::info(Debugger::GPU, "  height: {}", state.gp0.transfer.height);
}

static void copy_rectangle_vram_to_cpu(void) {
    uint16_t x = (state.gp0.buffer[1] >> 0) & UINT32_C(0xffff);
    uint16_t y = (state.gp0.buffer[1] >> 16) & UINT32_C(0xffff);
    uint16_t width = (state.gp0.buffer[2] >> 0) & UINT32_C(0xffff);;
    uint16_t height = (state.gp0.buffer[2] >> 16) & UINT32_C(0xffff);;

    debugger::info(Debugger::GPU, "VRAM to CPU transfer size: {}",
                   2 * width * height);

    state.hw.gpustat |= GPUSTAT_COPY_READY;
    state.gp0.state = GP0_COPY_VRAM_TO_CPU;
    state.gp0.transfer.x0 = x & UINT16_C(0x3ff);
    state.gp0.transfer.y0 = y & UINT16_C(0x1ff);
    state.gp0.transfer.width = ((width - 1) & UINT16_C(0x3ff)) + 1;
    state.gp0.transfer.height = ((height - 1) & UINT16_C(0x1ff)) + 1;
    state.gp0.transfer.x = 0;
    state.gp0.transfer.y = 0;
}

static void draw_mode_setting(void) {
    state.gpu.texture_page_x_base = (state.gp0.buffer[0] >> 0) & UINT32_C(0xf);
    state.gpu.texture_page_y_base = (state.gp0.buffer[0] >> 4) & UINT32_C(0x1);
    state.gpu.semi_transparency_mode = (state.gp0.buffer[0] >> 5) & UINT32_C(0x3);
    state.gpu.texture_page_colors = (state.gp0.buffer[0] >> 7) & UINT32_C(0x3);
    state.gpu.dither_enable = (state.gp0.buffer[0] >> 9) & UINT32_C(0x1);
    state.gpu.drawing_to_display_area_enable = (state.gp0.buffer[0] >> 10) & UINT32_C(0x1);
    state.gpu.texture_disable = (state.gp0.buffer[0] >> 11) & UINT32_C(0x1);
    state.gpu.textured_rectangle_x_flip = (state.gp0.buffer[0] >> 12) & UINT32_C(0x1);
    state.gpu.textured_rectangle_y_flip = (state.gp0.buffer[0] >> 13) & UINT32_C(0x1);

    state.hw.gpustat &= ~UINT32_C(0x87ff);
    state.hw.gpustat |= state.gp0.buffer[0] & UINT32_C(0x7ff);
    state.hw.gpustat |= (state.gp0.buffer[0] << 4) & UINT32_C(0x8000);
}

static void texture_window_setting(void) {
    uint32_t cmd = state.gp0.buffer[0];
    state.gpu.texture_window_mask_x = (cmd >> 0) & UINT32_C(0x1f);
    state.gpu.texture_window_mask_y = (cmd >> 5) & UINT32_C(0x1f);
    state.gpu.texture_window_offset_x = (cmd >> 10) & UINT32_C(0x1f);
    state.gpu.texture_window_offset_y = (cmd >> 15) & UINT32_C(0x1f);
}

static void set_drawing_area_top_left(void) {
    uint32_t cmd = state.gp0.buffer[0];
    state.gpu.drawing_area_x1 = (cmd >> 0) & UINT32_C(0x3ff);
    state.gpu.drawing_area_y1 = (cmd >> 10) & UINT32_C(0x3ff);
}

static void set_drawing_area_bottom_right(void) {
    uint32_t cmd = state.gp0.buffer[0];
    state.gpu.drawing_area_x2 = (cmd >> 0) & UINT32_C(0x3ff);
    state.gpu.drawing_area_y2 = (cmd >> 10) & UINT32_C(0x3ff);
}

static void set_drawing_offset(void) {
    uint32_t cmd = state.gp0.buffer[0];
    uint16_t offset_x = (cmd >>  0) & UINT32_C(0x7ff);
    uint16_t offset_y = (cmd >> 10) & UINT32_C(0x7ff);
    state.gpu.drawing_offset_x = sext_i11_i16(offset_x);
    state.gpu.drawing_offset_y = sext_i11_i16(offset_y);
}

static void mask_bit_setting(void) {
    uint32_t cmd = state.gp0.buffer[0];
    state.gpu.force_bit_mask = (cmd & UINT32_C(0x1)) != 0;
    state.gpu.check_bit_mask = (cmd & UINT32_C(0x2)) != 0;

    state.hw.gpustat &= ~UINT32_C(0x1800);
    state.hw.gpustat |= (cmd & UINT32_C(0x3)) << 11;
}

static struct {
    unsigned length;
    char const *name;
    void (*handler)();
} gp0_commands[256] = {
    { 1,  "nop", nop },
    { 1,  "clear_cache", nop },
    { 3,  "fill_rectangle", fill_rectangle },
    { 1,  "cmd_03", NULL },
    { 1,  "cmd_04", NULL },
    { 1,  "cmd_05", NULL },
    { 1,  "cmd_06", NULL },
    { 1,  "cmd_07", NULL },
    { 1,  "cmd_08", NULL },
    { 1,  "cmd_09", NULL },
    { 1,  "cmd_0a", NULL },
    { 1,  "cmd_0b", NULL },
    { 1,  "cmd_0c", NULL },
    { 1,  "cmd_0d", NULL },
    { 1,  "cmd_0e", NULL },
    { 1,  "cmd_0f", NULL },
    { 1,  "cmd_10", NULL },
    { 1,  "cmd_11", NULL },
    { 1,  "cmd_12", NULL },
    { 1,  "cmd_13", NULL },
    { 1,  "cmd_14", NULL },
    { 1,  "cmd_15", NULL },
    { 1,  "cmd_16", NULL },
    { 1,  "cmd_17", NULL },
    { 1,  "cmd_18", NULL },
    { 1,  "cmd_19", NULL },
    { 1,  "cmd_1a", NULL },
    { 1,  "cmd_1b", NULL },
    { 1,  "cmd_1c", NULL },
    { 1,  "cmd_1d", NULL },
    { 1,  "cmd_1e", NULL },
    { 1,  "interrupt_request", NULL },
    { 4,  "monochrome_3p_polygon_opaque",                             NULL },
    { 1,  "cmd_21", NULL },
    { 4,  "monochrome_3p_polygon_semi_transparent",                   NULL },
    { 1,  "cmd_23", NULL },
    { 7,  "textured_3p_polygon_opaque_texture_blending",             NULL },
    { 7,  "textured_3p_polygon_opaque_raw_texture",                  NULL },
    { 7,  "textured_3p_polygon_semi_transparent_textture_blending",  NULL },
    { 7,  "textured_3p_polygon_semi_transparent_raw_texture",        NULL },
    { 5,  "monochrome_4p_polygon_opaque", monochrome_4p_polygon_opaque },
    { 1,  "cmd_29", NULL },
    { 5,  "monochrome_4p_polygon_semi_transparent",                   NULL },
    { 1,  "cmd_2b", NULL },
    { 9,  "textured_4p_polygon_opaque_texture_blending",             NULL },
    { 9,  "textured_4p_polygon_opaque_raw_texture",                  NULL },
    { 9,  "textured_4p_polygon_semi_transparent_textture_blending",  NULL },
    { 9,  "textured_4p_polygon_semi_transparent_raw_texture",        NULL },
    { 6,  "shaded_3p_polygon_opaque", shaded_3p_polygon_opaque },
    { 1,  "cmd_31", NULL },
    { 6,  "shaded_3p_polygon_semi_transparent",                      NULL },
    { 1,  "cmd_33", NULL },
    { 9,  "shaded_3p_polygon_opaque_texture_blending", NULL },
    { 1,  "cmd_35", NULL },
    { 9,  "shaded_3p_polygon_semi_transparent_texture_blending", NULL },
    { 1,  "cmd_37", NULL },
    { 8,  "shaded_4p_polygon_opaque", shaded_4p_polygon_opaque },
    { 1,  "cmd_39", NULL },
    { 8,  "shaded_4p_polygon_semi_transparent",                         NULL },
    { 1,  "cmd_3b", NULL },
    { 12, "shaded_4p_polygon_opaque_texture_blending",                  NULL },
    { 1,  "cmd_3d", NULL },
    { 12, "shaded_4p_polygon_semi_transparent_texture_blending",        NULL },
    { 1,  "cmd_3f", NULL },
    { 3,  "monochrome_line_opaque", NULL },
    { 1,  "cmd_41", NULL },
    { 3,  "monochrome_line_semi_transparent", NULL },
    { 1,  "cmd_43", NULL },
    { 1,  "cmd_44", NULL },
    { 1,  "cmd_45", NULL },
    { 1,  "cmd_46", NULL },
    { 1,  "cmd_47", NULL },
    { 3,  "monochrome_polyline_opaque", NULL },
    { 1,  "cmd_49", NULL },
    { 3,  "monochrome_polyline_semi_transparent", NULL },
    { 1,  "cmd_4b", NULL },
    { 1,  "cmd_4c", NULL },
    { 1,  "cmd_4d", NULL },
    { 1,  "cmd_4e", NULL },
    { 1,  "cmd_4f", NULL },
    { 3,  "shaded_line_opaque", NULL },
    { 1,  "cmd_51", NULL },
    { 3,  "shaded_line_semi_transparent", NULL },
    { 1,  "cmd_53", NULL },
    { 1,  "cmd_54", NULL },
    { 1,  "cmd_55", NULL },
    { 1,  "cmd_56", NULL },
    { 1,  "cmd_57", NULL },
    { 3,  "shaded_polyline_opaque", NULL },
    { 1,  "cmd_59", NULL },
    { 3,  "shaded_polyline_semi_transparent", NULL },
    { 1,  "cmd_5b", NULL },
    { 1,  "cmd_5c", NULL },
    { 1,  "cmd_5d", NULL },
    { 1,  "cmd_5e", NULL },
    { 1,  "cmd_5f", NULL },
    { 3,  "monochrome_rectangle_variable_size_opaque", NULL },
    { 1,  "cmd_61", NULL },
    { 3,  "monochrome_rectangle_variable_size_semi_transparent", NULL },
    { 1,  "cmd_63", NULL },
    { 4,  "textured_rectangle_variable_size_opaque_texture_blending", NULL },
    { 4,  "textured_rectangle_variable_size_opaque_raw_texture", NULL },
    { 4,  "textured_rectangle_variable_size_semi_transparent_texture_blending", NULL },
    { 4,  "textured_rectangle_variable_size_semi_transparent_raw_texture", NULL },
    { 2,  "monochrome_rectangle_1x1_opaque", NULL },
    { 1,  "cmd_69", NULL },
    { 2,  "monochrome_rectangle_1x1_semi_transparent", NULL },
    { 1,  "cmd_6b", NULL },
    { 3,  "textured_rectangle_1x1_opaque_texture_blending", NULL },
    { 3,  "textured_rectangle_1x1_opaque_raw_texture", NULL },
    { 3,  "textured_rectangle_1x1_semi_transparent_texture_blending", NULL },
    { 3,  "textured_rectangle_1x1_semi_transparent_raw_texture", NULL },
    { 2,  "monochrome_rectangle_8x8_opaque", NULL },
    { 1,  "cmd_71", NULL },
    { 2,  "monochrome_rectangle_8x8_semi_transparent", NULL },
    { 1,  "cmd_73", NULL },
    { 3,  "textured_rectangle_8x8_opaque_texture_blending", NULL },
    { 3,  "textured_rectangle_8x8_opaque_raw_texture", NULL },
    { 3,  "textured_rectangle_8x8_semi_transparent_texture_blending", NULL },
    { 3,  "textured_rectangle_8x8_semi_transparent_raw_texture", NULL },
    { 2,  "monochrome_rectangle_16x16_opaque", NULL },
    { 1,  "cmd_79", NULL },
    { 2,  "monochrome_rectangle_16x16_semi_transparent", NULL },
    { 1,  "cmd_7b", NULL },
    { 3,  "textured_rectangle_16x16_opaque_texture_blending", NULL },
    { 3,  "textured_rectangle_16x16_opaque_raw_texture", NULL },
    { 3,  "textured_rectangle_16x16_semi_transparent_texture_blending", NULL },
    { 3,  "textured_rectangle_16x16_semi_transparent_raw_texture", NULL },
    { 4,  "copy_rectangle_vram_to_vram", NULL },
    { 1,  "cmd_81", NULL },
    { 1,  "cmd_82", NULL },
    { 1,  "cmd_83", NULL },
    { 1,  "cmd_84", NULL },
    { 1,  "cmd_85", NULL },
    { 1,  "cmd_86", NULL },
    { 1,  "cmd_87", NULL },
    { 1,  "cmd_88", NULL },
    { 1,  "cmd_89", NULL },
    { 1,  "cmd_8a", NULL },
    { 1,  "cmd_8b", NULL },
    { 1,  "cmd_8c", NULL },
    { 1,  "cmd_8d", NULL },
    { 1,  "cmd_8e", NULL },
    { 1,  "cmd_8f", NULL },
    { 1,  "cmd_90", NULL },
    { 1,  "cmd_91", NULL },
    { 1,  "cmd_92", NULL },
    { 1,  "cmd_93", NULL },
    { 1,  "cmd_94", NULL },
    { 1,  "cmd_95", NULL },
    { 1,  "cmd_96", NULL },
    { 1,  "cmd_97", NULL },
    { 1,  "cmd_98", NULL },
    { 1,  "cmd_99", NULL },
    { 1,  "cmd_9a", NULL },
    { 1,  "cmd_9b", NULL },
    { 1,  "cmd_9c", NULL },
    { 1,  "cmd_9d", NULL },
    { 1,  "cmd_9e", NULL },
    { 1,  "cmd_9f", NULL },
    { 3,  "copy_rectangle_cpu_to_vram", copy_rectangle_cpu_to_vram },
    { 1,  "cmd_a1", NULL },
    { 1,  "cmd_a2", NULL },
    { 1,  "cmd_a3", NULL },
    { 1,  "cmd_a4", NULL },
    { 1,  "cmd_a5", NULL },
    { 1,  "cmd_a6", NULL },
    { 1,  "cmd_a7", NULL },
    { 1,  "cmd_a8", NULL },
    { 1,  "cmd_a9", NULL },
    { 1,  "cmd_aa", NULL },
    { 1,  "cmd_ab", NULL },
    { 1,  "cmd_ac", NULL },
    { 1,  "cmd_ad", NULL },
    { 1,  "cmd_ae", NULL },
    { 1,  "cmd_af", NULL },
    { 1,  "cmd_b0", NULL },
    { 1,  "cmd_b1", NULL },
    { 1,  "cmd_b2", NULL },
    { 1,  "cmd_b3", NULL },
    { 1,  "cmd_b4", NULL },
    { 1,  "cmd_b5", NULL },
    { 1,  "cmd_b6", NULL },
    { 1,  "cmd_b7", NULL },
    { 1,  "cmd_b8", NULL },
    { 1,  "cmd_b9", NULL },
    { 1,  "cmd_ba", NULL },
    { 1,  "cmd_bb", NULL },
    { 1,  "cmd_bc", NULL },
    { 1,  "cmd_bd", NULL },
    { 1,  "cmd_be", NULL },
    { 1,  "cmd_bf", NULL },
    { 3,  "copy_rectangle_vram_to_cpu", copy_rectangle_vram_to_cpu },
    { 1,  "cmd_c1", NULL },
    { 1,  "cmd_c2", NULL },
    { 1,  "cmd_c3", NULL },
    { 1,  "cmd_c4", NULL },
    { 1,  "cmd_c5", NULL },
    { 1,  "cmd_c6", NULL },
    { 1,  "cmd_c7", NULL },
    { 1,  "cmd_c8", NULL },
    { 1,  "cmd_c9", NULL },
    { 1,  "cmd_ca", NULL },
    { 1,  "cmd_cb", NULL },
    { 1,  "cmd_cc", NULL },
    { 1,  "cmd_cd", NULL },
    { 1,  "cmd_ce", NULL },
    { 1,  "cmd_cf", NULL },
    { 1,  "cmd_d0", NULL },
    { 1,  "cmd_d1", NULL },
    { 1,  "cmd_d2", NULL },
    { 1,  "cmd_d3", NULL },
    { 1,  "cmd_d4", NULL },
    { 1,  "cmd_d5", NULL },
    { 1,  "cmd_d6", NULL },
    { 1,  "cmd_d7", NULL },
    { 1,  "cmd_d8", NULL },
    { 1,  "cmd_d9", NULL },
    { 1,  "cmd_da", NULL },
    { 1,  "cmd_db", NULL },
    { 1,  "cmd_dc", NULL },
    { 1,  "cmd_dd", NULL },
    { 1,  "cmd_de", NULL },
    { 1,  "cmd_df", NULL },
    { 1,  "cmd_e0", NULL },
    { 1,  "draw_mode_setting", draw_mode_setting },
    { 1,  "texture_window_setting", texture_window_setting },
    { 1,  "set_drawing_area_top_left", set_drawing_area_top_left },
    { 1,  "set_drawing_area_bottom_right", set_drawing_area_bottom_right },
    { 1,  "set_drawing_offset", set_drawing_offset },
    { 1,  "mask_bit_setting", mask_bit_setting },
    { 1,  "cmd_e7", NULL },
    { 1,  "cmd_e8", NULL },
    { 1,  "cmd_e9", NULL },
    { 1,  "cmd_ea", NULL },
    { 1,  "cmd_eb", NULL },
    { 1,  "cmd_ec", NULL },
    { 1,  "cmd_ed", NULL },
    { 1,  "cmd_ee", NULL },
    { 1,  "cmd_ef", NULL },
    { 1,  "cmd_f0", NULL },
    { 1,  "cmd_f1", NULL },
    { 1,  "cmd_f2", NULL },
    { 1,  "cmd_f3", NULL },
    { 1,  "cmd_f4", NULL },
    { 1,  "cmd_f5", NULL },
    { 1,  "cmd_f6", NULL },
    { 1,  "cmd_f7", NULL },
    { 1,  "cmd_f8", NULL },
    { 1,  "cmd_f9", NULL },
    { 1,  "cmd_fa", NULL },
    { 1,  "cmd_fb", NULL },
    { 1,  "cmd_fc", NULL },
    { 1,  "cmd_fd", NULL },
    { 1,  "cmd_fe", NULL },
    { 1,  "cmd_ff", NULL },
};

static void reset_gpu(uint32_t cmd) {
    state.hw.gpustat = UINT32_C(0x14802000);
    state.gp0.count = 0;
    state.gpu = gpu_registers();

    state.gpu.vertical_interlace = true;
    state.gpu.display_enable = false;
}

static void display_mode(uint32_t cmd) {
    //  0-1   Horizontal Resolution 1     (0=256, 1=320, 2=512, 3=640) ;GPUSTAT.17-18
    //  2     Vertical Resolution         (0=240, 1=480, when Bit5=1)  ;GPUSTAT.19
    //  3     Video Mode                  (0=NTSC/60Hz, 1=PAL/50Hz)    ;GPUSTAT.20
    //  4     Display Area Color Depth    (0=15bit, 1=24bit)           ;GPUSTAT.21
    //  5     Vertical Interlace          (0=Off, 1=On)                ;GPUSTAT.22
    //  6     Horizontal Resolution 2     (0=256/320/512/640, 1=368)   ;GPUSTAT.16
    //  7     "Reverseflag"               (0=Normal, 1=Distorted)      ;GPUSTAT.14
    //  8-23  Not used (zero)

    state.gpu.horizontal_resolution =
        ((cmd >> 0) & UINT8_C(0x3)) |
        ((cmd >> 4) & UINT8_C(0x1));
    state.gpu.vertical_resolution = (cmd >> 2) & UINT8_C(0x1);
    state.gpu.video_mode = (cmd >> 3) & UINT8_C(0x1);
    state.gpu.display_area_color_depth = (cmd >> 4) & UINT8_C(0x1);
    state.gpu.vertical_interlace = (cmd >> 5) & UINT32_C(0x1);

    state.hw.gpustat &= ~UINT32_C(0x007f4000);
    state.hw.gpustat |= (cmd & UINT32_C(0x3f)) << 17;
    state.hw.gpustat |= ((cmd >> 6) & UINT32_C(0x1)) << 16;
    state.hw.gpustat |= ((cmd >> 7) & UINT32_C(0x1)) << 14;
}

static void reset_command_buffer(uint32_t cmd) {
    state.gp0.count = 0;
    state.hw.gpustat |= GPUSTAT_CMD_READY;
    state.hw.gpustat |= GPUSTAT_DMA_READY;
    state.hw.gpustat &= GPUSTAT_COPY_READY;
}

static void ack_gpu_interrupt(uint32_t cmd) {
    state.hw.gpustat &= ~GPUSTAT_INT;
}

static void display_enable(uint32_t cmd) {
    state.gpu.display_enable = (cmd & UINT32_C(0x1)) == 0;

    state.hw.gpustat &= ~GPUSTAT_DISPLAY_DISABLE;
    state.hw.gpustat |= (cmd << 23) & GPUSTAT_DISPLAY_DISABLE;
}

static void dma_direction(uint32_t cmd) {
    state.gpu.dma_direction = cmd & UINT32_C(0x3);

    state.hw.gpustat &= ~UINT32_C(0x60000000);
    state.hw.gpustat |= (cmd & UINT32_C(0x3)) << 29;
}

static void start_of_display_area(uint32_t cmd) {
    state.gpu.start_of_display_area_x = (cmd >> 0) & UINT32_C(0x3ff);
    state.gpu.start_of_display_area_y = (cmd >> 10) & UINT32_C(0x1ff);
}

static void horizontal_display_range(uint32_t cmd) {
    state.gpu.horizontal_display_range_x1 = (cmd >> 0) & UINT32_C(0xfff);
    state.gpu.horizontal_display_range_x2 = (cmd >> 12) & UINT32_C(0xfff);
}

static void vertical_display_range(uint32_t cmd) {
    state.gpu.vertical_display_range_y1 = (cmd >> 0) & UINT32_C(0x3ff);
    state.gpu.vertical_display_range_y2 = (cmd >> 10) & UINT32_C(0x3ff);
}

static struct {
    char const *name;
    void (*handler)(uint32_t cmd);
} gp1_commands[64] = {
    { "reset_gpu",  reset_gpu },
    { "reset_command_buffer", reset_command_buffer },
    { "ack_gpu_interrupt", ack_gpu_interrupt },
    { "display_enable", display_enable },
    { "dma_direction", dma_direction },
    { "start_of_display_area", start_of_display_area },
    { "horizontal_display_range", horizontal_display_range },
    { "vertical_display_range", vertical_display_range },
    { "display_mode", display_mode },
    { "new_texture_disable", NULL },
    { "cmd_0a", NULL },
    { "cmd_0b", NULL },
    { "cmd_0c", NULL },
    { "cmd_0d", NULL },
    { "cmd_0e", NULL },
    { "cmd_0f", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "get_gpu_info", NULL },
    { "special_texture_disable", NULL },
    { "cmd_21", NULL },
    { "cmd_22", NULL },
    { "cmd_23", NULL },
    { "cmd_24", NULL },
    { "cmd_25", NULL },
    { "cmd_26", NULL },
    { "cmd_27", NULL },
    { "cmd_28", NULL },
    { "cmd_29", NULL },
    { "cmd_2a", NULL },
    { "cmd_2b", NULL },
    { "cmd_2c", NULL },
    { "cmd_2d", NULL },
    { "cmd_2e", NULL },
    { "cmd_2f", NULL },
    { "cmd_30", NULL },
    { "cmd_31", NULL },
    { "cmd_32", NULL },
    { "cmd_33", NULL },
    { "cmd_34", NULL },
    { "cmd_35", NULL },
    { "cmd_36", NULL },
    { "cmd_37", NULL },
    { "cmd_38", NULL },
    { "cmd_39", NULL },
    { "cmd_3a", NULL },
    { "cmd_3b", NULL },
    { "cmd_3c", NULL },
    { "cmd_3d", NULL },
    { "cmd_3e", NULL },
    { "cmd_3f", NULL },
};

static void gp0_command(uint32_t val) {
    unsigned index = state.gp0.count;
    state.gp0.buffer[index] = val;
    state.gp0.count++;

    uint8_t op_code = state.gp0.buffer[0] >> 24;
    unsigned cmd_length = gp0_commands[op_code].length;

    if (state.gp0.count == cmd_length) {
        debugger::info(Debugger::GPU, "{}", gp0_commands[op_code].name);
        state.gp0.count = 0;
        state.hw.gpustat |= GPUSTAT_CMD_READY;
        state.hw.gpustat |= GPUSTAT_DMA_READY;
        if (gp0_commands[op_code].handler == NULL) {
            psx::halt("unhandled GP0 command");
        } else {
            gp0_commands[op_code].handler();
        }
    } else {
        state.hw.gpustat &= ~GPUSTAT_CMD_READY;
        state.hw.gpustat &= ~GPUSTAT_DMA_READY;
    }
}

static void gp0_polyline(uint32_t val) {
    if (val == UINT32_C(0x55555555) ||
        val == UINT32_C(0x50005000)) {
        state.gp0.state = GP0_COMMAND;
        state.gp0.count = 0;
        state.hw.gpustat |= GPUSTAT_CMD_READY;
        state.hw.gpustat |= GPUSTAT_DMA_READY;
        state.hw.gpustat &= ~GPUSTAT_COPY_READY;
        return;
    }

    // TODO
/*    unsigned index = state.gp0.count;
    state.gp0.buffer[index] = val;
    state.gp0.count++;*/
}

static void gp0_copy_cpu_to_vram(uint32_t val) {
    uint32_t x = (state.gp0.transfer.x0 + state.gp0.transfer.x) & UINT16_C(0x3ff);
    uint32_t y = (state.gp0.transfer.y0 + state.gp0.transfer.y) & UINT16_C(0x1ff);
    uint16_t lo = val >> 0;
    uint16_t hi = val >> 16;

    memory::store_u16_le(state.vram + y * 2048 + 2 * x, lo);
    state.gp0.transfer.x++;
    x++;
    if (state.gp0.transfer.x >= state.gp0.transfer.width) {
        state.gp0.transfer.x = 0;
        state.gp0.transfer.y++;
        x = state.gp0.transfer.x0;
        y++;
    }

    memory::store_u16_le(state.vram + y * 2048 + 2 * x, hi);
    state.gp0.transfer.x++;
    if (state.gp0.transfer.x >= state.gp0.transfer.width) {
        state.gp0.transfer.x = 0;
        state.gp0.transfer.y++;
    }

    if (state.gp0.transfer.y >= state.gp0.transfer.height) {
        debugger::info(Debugger::GPU, "CPU to VRAM transfer complete");
        state.gp0.state = GP0_COMMAND;
        state.gp0.count = 0;
        state.hw.gpustat |= GPUSTAT_CMD_READY;
        state.hw.gpustat |= GPUSTAT_DMA_READY;
        state.hw.gpustat &= ~GPUSTAT_COPY_READY;
    }
}

static void gp0_copy_vram_to_cpu(uint32_t val) {
    psx::halt("gp0_copy_vram_to_cpu");
}

void write_gpu0(uint32_t val) {
    debugger::debug(Debugger::GPU, "gpu0 <- {:08x}", val);

    switch (state.gp0.state) {
    case GP0_COMMAND:           gp0_command(val); break;
    case GP0_POLYLINE:          gp0_polyline(val); break;
    case GP0_COPY_CPU_TO_VRAM:  gp0_copy_cpu_to_vram(val); break;
    case GP0_COPY_VRAM_TO_CPU:  gp0_copy_vram_to_cpu(val); break;
    }
}

void write_gpu1(uint32_t val) {
    debugger::info(Debugger::GPU, "gpu1 <- {:08x}", val);

    uint8_t op_code = (val >> 24) & UINT8_C(0x3f);
    debugger::info(Debugger::GPU, "{}", gp1_commands[op_code].name);
    if (gp1_commands[op_code].handler == NULL) {
        psx::halt("unhandled GP1 command");
    } else {
        gp1_commands[op_code].handler(val);
    }
}

void hblank_event() {
    // CPU Clock:
    // - 33.868800MHz (44100Hz*300h)
    // Video Clock
    // - 53.222400MHz (44100Hz*300h*11/7)
    //
    // Vertical Timings
    // - PAL:  314 scanlines per frame (13Ah)
    // - NTSC: 263 scanlines per frame (107h)
    //
    // Horizontal Timings
    // - PAL:  3406 video cycles per scanline (or 3406.1 or so?)
    // - NTSC: 3413 video cycles per scanline (or 3413.6 or so?)

    bool pal = state.gpu.video_mode != 0;
    bool interlace = state.gpu.vertical_interlace;
    bool resolution = state.gpu.vertical_resolution != 0;
    unsigned scanline_vblank = 240;
    unsigned scanline_endframe = pal ? 314 : 263;

    unsigned long cpu_clock = state.cycles;
    unsigned long delay = (pal ? 3406 : 3413) * 7 / 11;

    state.gpu.scanline++;
    state.hw.gpustat &= ~GPUSTAT_VBLANK;

    if (state.gpu.scanline >= scanline_endframe) {
        state.gpu.scanline = 0;
        state.gpu.frame++;
    }

    if (state.gpu.scanline < scanline_vblank) {
        bool bit31_set = (resolution && interlace) ?
            state.gpu.frame % 2 :
            state.gpu.scanline % 2;

        if (bit31_set) {
            state.hw.gpustat |= GPUSTAT_VBLANK;
        }
    }

    if (state.gpu.scanline == scanline_vblank) {
        hw::set_i_stat(I_STAT_VBLANK);
        refreshVideoImage();
    }

    state.schedule_event(cpu_clock + delay, hblank_event);
}

};  // psx::hw
