
#include <cstring>
#include <iostream>
#include <fstream>

#include <cxxopts.hpp>
#include <fmt/format.h>

#include <psx/debugger.h>
#include <psx/psx.h>
#include <psx/memory.h>
#include <psx/gui.h>

int main(int argc, char *argv[])
{
    cxxopts::Options options("ps1", "PS1 console emulator");
    options.add_options()
        ("record",      "Record execution trace", cxxopts::value<std::string>())
        ("replay",      "Replay execution trace", cxxopts::value<std::string>())
        ("recompiler",  "Enable recompiler", cxxopts::value<bool>()->default_value("false"))
        ("b,bios",      "Select BIOS rom", cxxopts::value<std::string>())
        ("c,cd-rom",    "CD-ROM file", cxxopts::value<std::string>())
        ("h,help",      "Print usage");
    options.parse_positional("rom");
    options.positional_help("FILE");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    if (result.count("cd-rom") == 0) {
        std::cout << "CD-ROM file unspecified" << std::endl;
        std::cout << options.help() << std::endl;
        exit(1);
    }

    if (result.count("bios") == 0) {
        std::cout << "BIOS file unspecified" << std::endl;
        std::cout << options.help() << std::endl;
        exit(1);
    }

    std::string rom_file = result["cd-rom"].as<std::string>();
    std::ifstream cd_rom_contents(rom_file);
    if (!cd_rom_contents.good()) {
        fmt::print("CD-ROM file '{}' not found\n", rom_file);
        std::cout << options.help() << std::endl;
        exit(1);
    }

    std::string bios_file = result["bios"].as<std::string>();
    std::ifstream bios_contents(bios_file);
    if (!bios_contents.good()) {
        fmt::print("BIOS file '{}' not found\n", bios_file);
        std::cout << options.help() << std::endl;
        exit(1);
    }

    debugger::debugger.load_settings();
    psx::state.load_bios(bios_contents);
    psx::state.load_cd_rom(cd_rom_contents);
    cd_rom_contents.close();
    bios_contents.close();
    psx::start_gui();
    return 0;
}
