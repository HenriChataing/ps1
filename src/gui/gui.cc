
#include <chrono>
#include <cinttypes>
#include <cstdio>
#include <ctime>
#include <vector>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <imgui_disassembler.h>
#include <imgui_memory_editor.h>
#include <imgui_trace.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <psx/psx.h>
#include <psx/debugger.h>
#include <psx/gui.h>
#include <assembly/registers.h>
#include <graphics.h>

using namespace psx;

namespace psx {

typedef void (*ExternalWindowRenderer)();
static std::vector<ExternalWindowRenderer> externalWindowRenderers;

static Disassembler ramDisassembler(24);
static Disassembler biosDisassembler(24);
static MemoryEditor cdromMemory;
static Trace cpuTrace(&debugger::debugger.cpu_trace);

static std::chrono::time_point<std::chrono::steady_clock> startTime;
static unsigned long startCycles;

static void glfwErrorCallback(int error, const char* description) {
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

static void ShowAnalytics(void) {
    // CPU freq is 33.87 MHz
    static float timeRatio[5 * 60] = { 0 };
    static unsigned plotOffset = 0;
    unsigned plotLength = 5 * 60;
    unsigned plotUpdateInterval = 200;
    float plotWidth = ImGui::GetContentRegionAvail().x;
    float plotHeight = 40.0f;
    ImVec2 plotDimensions = ImVec2(plotWidth, plotHeight);

    auto updateTime = std::chrono::steady_clock::now();;
    std::chrono::duration<double> diffTime = updateTime - startTime;
    unsigned long updateCycles = psx::state.cycles;

    float elapsedMilliseconds = diffTime.count() * 1000.0;
    float machineMilliseconds = (updateCycles - startCycles) / 33870.0;

    if (elapsedMilliseconds >= plotUpdateInterval) {
        timeRatio[plotOffset] =
            machineMilliseconds * 100.0 / elapsedMilliseconds;

        plotOffset = (plotOffset + 1) % plotLength;
        startTime = updateTime;
        startCycles = updateCycles;
    }

    ImGui::PlotLines("", timeRatio, plotLength, plotOffset,
        "time ratio", 0.0f, 100.0f, plotDimensions);
}

static void ShowCpuRegisters(void) {
    ImGui::Text("pc       %08" PRIx32 "\n", psx::state.cpu.pc);
    for (unsigned int i = 0; i < 32; i+=2) {
        ImGui::Text("%-8.8s %08" PRIx32 "  %-8.8s %08" PRIx32 "\n",
            assembly::cpu::getRegisterName(i), psx::state.cpu.gpr[i],
            assembly::cpu::getRegisterName(i + 1), psx::state.cpu.gpr[i + 1]);
    }
}

static void ShowCp0Registers(void) {
    #define PrintReg(fmt, name) \
        ImGui::Text("%-8.8s %08" fmt "\n", \
            #name, psx::state.cp0.name)
    #define Print2Regs(fmt0, name0, fmt1, name1) \
        ImGui::Text("%-8.8s %08" fmt0 "  %-8.8s %08" fmt1 "\n", \
            #name0, psx::state.cp0.name0, \
            #name1, psx::state.cp0.name1)

    PrintReg(PRIx32, prid);
    Print2Regs(PRIx32, bpc,      PRIx32, bda);
    Print2Regs(PRIx32, bdam,     PRIx32, bpcm);
    Print2Regs(PRIx32, jumpdest, PRIx32, dcic);
    Print2Regs(PRIx32, sr,       PRIx32, cause);
    Print2Regs(PRIx32, epc,      PRIx32, badvaddr);

    #undef PrintReg
    #undef Print2Regs
}

static void ShowCp2Registers(void) {
}

static void ShowMemoryControlRegisters(void) {
    ImGui::Text("expansion_1_base_addr      %08" PRIx32 "\n",
        psx::state.hw.expansion_1_base_addr);
    ImGui::Text("expansion_2_base_addr      %08" PRIx32 "\n",
        psx::state.hw.expansion_2_base_addr);
    ImGui::Text("expansion_1_delay_size     %08" PRIx32 "\n",
        psx::state.hw.expansion_1_delay_size);
    ImGui::Text("expansion_3_delay_size     %08" PRIx32 "\n",
        psx::state.hw.expansion_3_delay_size);
    ImGui::Text("bios_rom_delay_size        %08" PRIx32 "\n",
        psx::state.hw.bios_rom_delay_size);
    ImGui::Text("spu_delay                  %08" PRIx32 "\n",
        psx::state.hw.spu_delay);
    ImGui::Text("cdrom_delay                %08" PRIx32 "\n",
        psx::state.hw.cdrom_delay);
    ImGui::Text("expansion_2_delay_size     %08" PRIx32 "\n",
        psx::state.hw.expansion_2_delay_size);
    ImGui::Text("common_delay               %08" PRIx32 "\n",
        psx::state.hw.common_delay);
    ImGui::Text("ram_size                   %08" PRIx32 "\n",
        psx::state.hw.ram_size);
    ImGui::Text("cache_control              %08" PRIx32 "\n",
        psx::state.hw.cache_control);
}

static void ShowJoyControlRegisters(void) {
    ImGui::Text("joy_stat                   %04" PRIx16 "\n",
        psx::state.hw.joy_stat);
    ImGui::Text("joy_mode                   %04" PRIx16 "\n",
        psx::state.hw.joy_mode);
    ImGui::Text("joy_ctrl                   %04" PRIx16 "\n",
        psx::state.hw.joy_ctrl);
    ImGui::Text("joy_baud                   %04" PRIx16 "\n",
        psx::state.hw.joy_baud);
}

static void ShowInterruptControlRegisters(void) {
    ImGui::Text("i_stat                     %04" PRIx16 "\n",
        psx::state.hw.i_stat);
    ImGui::Text("i_mask                     %04" PRIx16 "\n",
        psx::state.hw.i_mask);
}

static void ShowTimerRegisters(void) {
    ImGui::Text("tim0_value                 %04" PRIx16 "\n",
        psx::state.hw.timer[0].value);
    ImGui::Text("tim0_mode                  %04" PRIx16 "\n",
        psx::state.hw.timer[0].mode);
    ImGui::Text("tim0_target                %04" PRIx16 "\n",
        psx::state.hw.timer[0].target);
    ImGui::Text("tim1_value                 %04" PRIx16 "\n",
        psx::state.hw.timer[1].value);
    ImGui::Text("tim1_mode                  %04" PRIx16 "\n",
        psx::state.hw.timer[1].mode);
    ImGui::Text("tim1_target                %04" PRIx16 "\n",
        psx::state.hw.timer[1].target);
    ImGui::Text("tim2_value                 %04" PRIx16 "\n",
        psx::state.hw.timer[2].value);
    ImGui::Text("tim2_mode                  %04" PRIx16 "\n",
        psx::state.hw.timer[2].mode);
    ImGui::Text("tim2_target                %04" PRIx16 "\n",
        psx::state.hw.timer[2].target);
}

static void ShowDMARegisters(void) {
    ImGui::Text("dpcr                       %08" PRIx32 "\n",
        psx::state.hw.dpcr);
    ImGui::Text("dicr                       %08" PRIx32 "\n",
        psx::state.hw.dicr);
}

static void ShowCDROMRegisters(void) {
    ImGui::Text("index                      %02" PRIx8 "\n",
        psx::state.cdrom.index);
    ImGui::Text("command                    %02" PRIx8 "\n",
        psx::state.cdrom.command);
    ImGui::Text("request                    %02" PRIx8 "\n",
        psx::state.cdrom.request);

    ImGui::Text("interrupt_enable           %02" PRIx8 "\n",
        psx::state.cdrom.interrupt_enable);
    ImGui::Text("interrupt_flag             %02" PRIx8 "\n",
        psx::state.cdrom.interrupt_flag);

    ImGui::Text("parameter fifo [%d]\n",
        psx::state.cdrom.parameter_fifo_index);
    for (unsigned i = 0; i < 16; i+=4) {
        ImGui::Text("   %02x  %02x  %02x  %02x\n",
            psx::state.cdrom.parameter_fifo[i + 0],
            psx::state.cdrom.parameter_fifo[i + 1],
            psx::state.cdrom.parameter_fifo[i + 2],
            psx::state.cdrom.parameter_fifo[i + 3]);
    }

    ImGui::Text("response fifo [%d]\n",
        psx::state.cdrom.response_fifo_index);
    for (unsigned i = 0; i < 16; i+=4) {
        ImGui::Text("   %02x  %02x  %02x  %02x\n",
            psx::state.cdrom.response_fifo[i + 0],
            psx::state.cdrom.response_fifo[i + 1],
            psx::state.cdrom.response_fifo[i + 2],
            psx::state.cdrom.response_fifo[i + 3]);
    }
}

static void ShowGPURegisters(void) {
    ImGui::Text("scanline: %u, frame: %u\n",
        psx::state.gpu.scanline, psx::state.gpu.frame);
    ImGui::Text("gpustat                    %08" PRIx32 "\n",
        psx::state.hw.gpustat);

    uint32_t op_code = state.gp0.buffer[0] >> 24;
    ImGui::Text("GP0 op_code %02" PRIx32 "\n",
        state.gp0.count > 0 ? op_code : 0);
    ImGui::Text("GP0 buffer\n");
    for (unsigned i = 0; i < state.gp0.count; i++) {
        ImGui::Text("    %08" PRIx32 "\n", state.gp0.buffer[i]);
    }

    ImGui::Text("horizontal_resolution          %" PRIu8 "\n",
        psx::state.gpu.horizontal_resolution);
    ImGui::Text("vertical_resolution            %" PRIu8 "\n",
        psx::state.gpu.vertical_resolution);
    ImGui::Text("video_mode                     %" PRIu8 "\n",
        psx::state.gpu.video_mode);
    ImGui::Text("display_area_color_depth       %" PRIu8 "\n",
        psx::state.gpu.display_area_color_depth);
    ImGui::Text("vertical_interlace             %s\n",
        psx::state.gpu.vertical_interlace ? "true" : "false");
    ImGui::Text("dma_direction                  %" PRIu8 "\n",
        psx::state.gpu.dma_direction);
    ImGui::Text("start_of_display_area_x        %" PRIu16 "\n",
        psx::state.gpu.start_of_display_area_x);
    ImGui::Text("start_of_display_area_y        %" PRIu16 "\n",
        psx::state.gpu.start_of_display_area_y);
    ImGui::Text("horizontal_display_range       %" PRIu16 " - %" PRIu16 "\n",
        psx::state.gpu.horizontal_display_range_x1,
        psx::state.gpu.horizontal_display_range_x2);
    ImGui::Text("vertical_display_range         %" PRIu16 " - %" PRIu16 "\n",
        psx::state.gpu.vertical_display_range_y1,
        psx::state.gpu.vertical_display_range_y2);
    ImGui::Text("texture_disable                %s\n",
        psx::state.gpu.texture_disable ? "true" : "false");
    ImGui::Text("dither_enable                  %s\n",
        psx::state.gpu.dither_enable ? "true" : "false");
    ImGui::Text("drawing_to_display_area_enable %s\n",
        psx::state.gpu.drawing_to_display_area_enable ? "true" : "false");
    ImGui::Text("semi_transparency_mode         %" PRIu8 "\n",
        psx::state.gpu.semi_transparency_mode);
    ImGui::Text("force_bit_mask                 %s\n",
        psx::state.gpu.force_bit_mask ? "true" : "false");
    ImGui::Text("check_bit_mask                 %s\n",
        psx::state.gpu.check_bit_mask ? "true" : "false");
    ImGui::Text("texture_page_x_base            %" PRIu8 "\n",
        psx::state.gpu.texture_page_x_base);
    ImGui::Text("texture_page_y_base            %" PRIu8 "\n",
        psx::state.gpu.texture_page_y_base);
    ImGui::Text("texture_page_colors            %" PRIu8 "\n",
        psx::state.gpu.texture_page_colors);
    ImGui::Text("textured_rectangle_x_flip      %s\n",
        psx::state.gpu.textured_rectangle_x_flip ? "true" : "false");
    ImGui::Text("textured_rectangle_y_flip      %s\n",
        psx::state.gpu.textured_rectangle_y_flip ? "true" : "false");
    ImGui::Text("texture_window_mask_x          %" PRIu8 "\n",
        psx::state.gpu.texture_window_mask_x);
    ImGui::Text("texture_window_mask_y          %" PRIu8 "\n",
        psx::state.gpu.texture_window_mask_y);
    ImGui::Text("texture_window_offset_x        %" PRIu8 "\n",
        psx::state.gpu.texture_window_offset_x);
    ImGui::Text("texture_window_offset_y        %" PRIu8 "\n",
        psx::state.gpu.texture_window_offset_y);
    ImGui::Text("drawing_area_x1                %" PRId16 "\n",
        psx::state.gpu.drawing_area_x1);
    ImGui::Text("drawing_area_y1                %" PRId16 "\n",
        psx::state.gpu.drawing_area_y1);
    ImGui::Text("drawing_area_x2                %" PRId16 "\n",
        psx::state.gpu.drawing_area_x2);
    ImGui::Text("drawing_area_y2                %" PRId16 "\n",
        psx::state.gpu.drawing_area_y2);
    ImGui::Text("drawing_offset_x               %" PRId16 "\n",
        psx::state.gpu.drawing_offset_x);
    ImGui::Text("drawing_offset_y               %" PRId16 "\n",
        psx::state.gpu.drawing_offset_y);
}

struct Module {
    char const *Name;
    int Label;
    void (*ShowInformation)();
};

static Module Modules[] = {
    { "Analytics",      -1,                     ShowAnalytics },
    { "CPU",            Debugger::CPU,          ShowCpuRegisters },
    { "CPU::COP0",      Debugger::COP0,         ShowCp0Registers },
    { "CPU::COP2",      Debugger::COP2,         ShowCp2Registers },
    { "HW::MC",         Debugger::MC,           ShowMemoryControlRegisters },
    { "HW::JC",         Debugger::JC,           ShowJoyControlRegisters },
    { "HW::IC",         Debugger::IC,           ShowInterruptControlRegisters },
    { "HW::DMA",        Debugger::DMA,          ShowDMARegisters },
    { "HW::CDROM",      Debugger::CDROM,        ShowCDROMRegisters },
    { "HW::GPU",        Debugger::GPU,          ShowGPURegisters },
    { "HW::Timer",      Debugger::Timer,        ShowTimerRegisters },
};

static void ShowScreen(bool *show_screen) {
    size_t width;
    size_t height;
    GLuint texture;

    if (getVideoImage(&width, &height, &texture)) {
        ImGui::SetNextWindowSize(ImVec2(width + 15, height + 35));
        ImGui::Begin("Screen", show_screen);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        ImGui::GetWindowDrawList()->AddImage(
            (void *)(unsigned long)texture, pos,
            ImVec2(pos.x + width, pos.y + height),
            ImVec2(0, 0), ImVec2(1, 1));
        ImGui::End();
    } else {
        ImGui::Begin("Screen", show_screen);
        ImGui::Text("Framebuffer invalid");
        ImGui::End();
    }
}

static void ShowLogConfig(bool *show_log_config) {
    ImGui::Begin("Log Config", show_log_config);
    for (int label = 0; label < Debugger::LabelCount; label++) {
        int verb = debugger::debugger.verbosity[label];
        float col[3] = {
            (float)debugger::debugger.color[label].r / 256.f,
            (float)debugger::debugger.color[label].g / 256.f,
            (float)debugger::debugger.color[label].b / 256.f, };
        ImGui::ColorEdit3("Log color", col,
            ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
        ImGui::SameLine();
        ImGui::SetNextItemWidth(100);
        ImGui::Combo(debugger::LabelName[label],
            &verb, "none\0error\0warn\0info\0debug\0\0");
        debugger::debugger.verbosity[label] = (Debugger::Verbosity)verb;
        debugger::debugger.color[label].r = (uint8_t)(col[0] * 256.f);
        debugger::debugger.color[label].g = (uint8_t)(col[1] * 256.f);
        debugger::debugger.color[label].b = (uint8_t)(col[2] * 256.f);
    }
    ImGui::End();
}

static void ShowDisassembler(bool *show_disassembler) {
    ImGui::Begin("Disassembler", show_disassembler);
    if (ImGui::BeginTabBar("Memory", 0)) {
        if (ImGui::BeginTabItem("RAM")) {
            ramDisassembler.DrawContents(
                assembly::cpu::disassemble,
                psx::state.ram, sizeof(psx::state.ram),
                psx::state.cpu.pc,
                0x80000000,
                true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("BIOS")) {
            biosDisassembler.DrawContents(
                assembly::cpu::disassemble,
                psx::state.bios, sizeof(psx::state.bios),
                psx::state.cpu.pc,
                0xBFC00000,
                true);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Cd-ROM")) {
            cdromMemory.DrawContents(
                psx::state.cd_rom,
                psx::state.cd_rom_size,
                0x1f000000);
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

static void ShowTrace(bool *show_trace) {
    ImGui::Begin("Trace", show_trace);
    if (ImGui::Button("Clear traces")) {
        debugger::debugger.cpu_trace.reset();
    }
    if (ImGui::BeginTabBar("Trace", 0)) {
        if (ImGui::BeginTabItem("Cpu")) {
            if (psx::halted()) {
                cpuTrace.DrawContents("cpu", assembly::cpu::disassemble);
            } else {
                ImGui::Text("Cpu is running...");
            }
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

static void ShowBreakpoints(bool *show_breakpoints) {
    static char addr_input_buf[32];
    bool added = false;
    bool removed = false;
    unsigned removed_id = 0;

    ImGui::Begin("Breakpoints", show_breakpoints);
    added |= ImGui::InputText("##addr", addr_input_buf, 32,
        ImGuiInputTextFlags_CharsHexadecimal |
        ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    added |= ImGui::Button("Add");

    if (added) {
        uint32_t addr;
        if (sscanf(addr_input_buf, "%" PRIx32 "X", &addr) == 1) {
            debugger::debugger.set_breakpoint(addr);
        }
    }

    ImGui::BeginChild("BreakpointList");
    auto it = debugger::debugger.breakpointsBegin();
    for (; it != debugger::debugger.breakpointsEnd(); it++) {
        ImGui::PushID(it->first);
        ImGui::Text("#%-2u", it->first);
        ImGui::SameLine();
        ImGui::Checkbox("", &it->second.enabled);
        ImGui::SameLine();
        if (ImGui::Button("Remove")) {
            removed = true;
            removed_id = it->first;
        }
        ImGui::SameLine();
        ImGui::Text("%08lx", it->second.addr);
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::End();

    if (removed) {
        debugger::debugger.unset_breakpoint(removed_id);
    }
}

static void ShowWatchpoints(bool *show_watchpoints) {
    static char start_addr_input_buf[32];
    static char end_addr_input_buf[32];
    bool added = false;
    bool removed = false;
    unsigned removed_id = 0;

    ImGui::Begin("Watchpoints", show_watchpoints);
    added |= ImGui::InputText("##start_addr", start_addr_input_buf, 32,
        ImGuiInputTextFlags_CharsHexadecimal |
        ImGuiInputTextFlags_EnterReturnsTrue);
    // ImGui::SameLine();
    added |= ImGui::InputText("##end_addr", end_addr_input_buf, 32,
        ImGuiInputTextFlags_CharsHexadecimal |
        ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    added |= ImGui::Button("Add");

    if (added) {
        uint32_t start_addr, end_addr;
        if (sscanf(start_addr_input_buf, "%" PRIx32 "X", &start_addr) == 1 &&
            sscanf(end_addr_input_buf, "%" PRIx32 "X", &end_addr) == 1) {
            debugger::debugger.set_watchpoint(start_addr, end_addr);
        }
    }

    ImGui::BeginChild("WatchpointList");
    auto it = debugger::debugger.watchpointsBegin();
    for (; it != debugger::debugger.watchpointsEnd(); it++) {
        ImGui::PushID(it->first);
        ImGui::Text("#%-2u", it->first);
        ImGui::SameLine();
        ImGui::Checkbox("", &it->second.enabled);
        ImGui::SameLine();
        if (ImGui::Button("Remove")) {
            removed = true;
            removed_id = it->first;
        }
        ImGui::SameLine();
        ImGui::Text("%08lx %08lx", it->second.start_addr, it->second.end_addr);
        ImGui::PopID();
    }
    ImGui::EndChild();
    ImGui::End();

    if (removed) {
        debugger::debugger.unset_watchpoint(removed_id);
    }
}

static void ShowDebuggerWindow(void) {
    static bool show_screen = true;
    static bool show_log_config = false;
    static bool show_disassembler = true;
    static bool show_trace = false;
    static bool show_breakpoints = false;
    static bool show_watchpoints = false;

    if (show_screen) ShowScreen(&show_screen);
    if (show_log_config) ShowLogConfig(&show_log_config);
    if (show_disassembler) ShowDisassembler(&show_disassembler);
    if (show_trace) ShowTrace(&show_trace);
    if (show_breakpoints) ShowBreakpoints(&show_breakpoints);
    if (show_watchpoints) ShowWatchpoints(&show_watchpoints);

    if (ImGui::Begin("Debugger", NULL, ImGuiWindowFlags_MenuBar)) {
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Load")) {}
                if (ImGui::BeginMenu("Export")) {
                    if (ImGui::MenuItem("cpu trace")) {}
                    if (ImGui::MenuItem("dram disassembly")) {}
                    if (ImGui::MenuItem("imem disassembly")) {}
                    ImGui::EndMenu();
                }
                if (ImGui::MenuItem("Save screen")) {
                    // TODO
                    // exportAsPNG("screen.png");
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("View")) {
                if (ImGui::MenuItem("Screen", NULL, &show_screen)) {}
                if (ImGui::MenuItem("Disassembler", NULL, &show_disassembler)) {}
                if (ImGui::MenuItem("Trace", NULL, &show_trace)) {}
                if (ImGui::MenuItem("Breakpoints", NULL, &show_breakpoints)) {}
                if (ImGui::MenuItem("Watchpoints", NULL, &show_watchpoints)) {}
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Options")) {
                if (ImGui::MenuItem("Log", NULL, &show_log_config)) {}
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        ImGui::Text("Real time: %lums (%lu)\n",
            psx::state.cycles / 33870lu, psx::state.cycles);

        if (psx::halted()) {
            ImGui::Text("Machine halt reason: '%s'\n",
                psx::halted_reason().c_str());
            if (ImGui::Button("Reset")) { psx::reset(); }
            ImGui::SameLine();
            if (ImGui::Button("Continue")) { psx::resume(); }
            ImGui::SameLine();
            if (ImGui::Button("Step")) { psx::step(); }
        } else {
            if (ImGui::Button("Halt")) {
                psx::halt("Interrupted by user");
            }
        }

        static int selected = 0;
        ImGui::Separator();
        ImGui::BeginChild("module select", ImVec2(150, 0), true);
        for (int i = 0; i < IM_ARRAYSIZE(Modules); i++) {
            if (ImGui::Selectable(Modules[i].Name, selected == i)) {
                selected = i;
            }
        }
        ImGui::EndChild();
        ImGui::SameLine();

        ImGui::BeginChild("module view",
            ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));
        ImGui::Text("%s", Modules[selected].Name);
        if (Modules[selected].Label < Debugger::LabelCount &&
            Modules[selected].Label >= 0) {
            Debugger::Label label = (Debugger::Label)Modules[selected].Label;
            int verb = debugger::debugger.verbosity[label];
            float col[3] = {
                (float)debugger::debugger.color[label].r / 256.f,
                (float)debugger::debugger.color[label].g / 256.f,
                (float)debugger::debugger.color[label].b / 256.f, };
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
            ImGui::ColorEdit3("Log color", col,
                ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel);
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 125);
            ImGui::SetNextItemWidth(100);
            std::string ComboID = "verbosity_" + label;
            ImGui::Combo(ComboID.c_str(), &verb, "none\0error\0warn\0info\0debug\0\0");
            debugger::debugger.verbosity[label] = (Debugger::Verbosity)verb;
            debugger::debugger.color[label].r = (uint8_t)(col[0] * 256.f);
            debugger::debugger.color[label].g = (uint8_t)(col[1] * 256.f);
            debugger::debugger.color[label].b = (uint8_t)(col[2] * 256.f);
        }
        ImGui::Separator();
        ImGui::BeginChild("module info");
        Modules[selected].ShowInformation();
        ImGui::EndChild();
        ImGui::EndChild();

        ImGui::End();
    }
}

/**
 * Capture key callbacks, interpret them as game inputs.
 */
void joyKeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
#if 0
    unsigned keyval;
    switch (action) {
    case GLFW_PRESS: keyval = 1; break;
    case GLFW_RELEASE: keyval = 0; break;
    default: return;
    }

    psx::controller *controller = psx::state.controllers[activeController];
    if (controller == NULL) {
        return;
    }

    switch (key) {
    case GLFW_KEY_A:     controller->A               = keyval; break;
    case GLFW_KEY_B:     controller->B               = keyval; break;
    case GLFW_KEY_Z:     controller->Z               = keyval; break;
    case GLFW_KEY_SPACE: controller->start           = keyval; break;
    case GLFW_KEY_UP:    controller->direction_up    = keyval; break;
    case GLFW_KEY_DOWN:  controller->direction_down  = keyval; break;
    case GLFW_KEY_LEFT:  controller->direction_left  = keyval; break;
    case GLFW_KEY_RIGHT: controller->direction_right = keyval; break;
    case GLFW_KEY_L:     controller->L               = keyval; break;
    case GLFW_KEY_R:     controller->R               = keyval; break;
    case GLFW_KEY_U:     controller->camera_up       = keyval; break;
    case GLFW_KEY_J:     controller->camera_down     = keyval; break;
    case GLFW_KEY_H:     controller->camera_left     = keyval; break;
    case GLFW_KEY_K:     controller->camera_right    = keyval; break;
    default: break;
    }

    controller->direction_x =
        127 * controller->direction_right -
        127 * controller->direction_left;
    controller->direction_y =
        127 * controller->direction_up -
        127 * controller->direction_down;
#endif
}

void addWindowRenderer(void (*renderer)()) {
    externalWindowRenderers.push_back(renderer);
}

int start_gui()
{
    // Initialize the machine state.
    psx::state.reset();
    startTime = std::chrono::steady_clock::now();
    startCycles = 0;

    // Start interpreter thread.
    psx::start();

    // Setup window
    glfwSetErrorCallback(glfwErrorCallback);
    if (!glfwInit())
        return 1;

    // GL 3.0 + GLSL 130
    const char* glsl_version = "#version 130";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    // Create window with graphics context
    GLFWwindow* window = glfwCreateWindow(
        1280, 720, "Nintendo 64 Emulation", NULL, NULL);
    if (window == NULL)
        return 1;
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Initialize OpenGL loader
    bool err = glewInit() != GLEW_OK;
    if (err) {
        fprintf(stderr, "Failed to initialize OpenGL loader!\n");
        return 1;
    }

    glfwSetKeyCallback(window, joyKeyCallback);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // Setup Platform/Renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
    ImFont *font = io.Fonts->AddFontFromFileTTF("src/gui/VeraMono.ttf", 13);
    if (!font)
        return 1;
    // io.Fonts->GetTexDataAsRGBA32();

    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    // Our state
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Main loop
    while (!glfwWindowShouldClose(window))
    {
        // Poll and handle events (inputs, window resize, etc.)
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Show the main debugger control window.
        // Displays Continue, Halt commands and register values.
        ShowDebuggerWindow();

        // Call registered window renderers.
        for (ExternalWindowRenderer renderer: externalWindowRenderers) {
            renderer();
        }

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x, clear_color.y, clear_color.z, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();

    psx::stop();
    return 0;
}

}; /* namespace psx */