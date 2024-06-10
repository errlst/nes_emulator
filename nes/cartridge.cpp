#include "cartridge.h"
#include <string_view>

cartridge::cartridge(const std::string &filename) {
    ifs_.open(filename, std::ifstream::binary);
    if (ifs_) {
        valid_ = load();
    }
}

auto cartridge::load() -> bool {
    return load_header() && load_trainer() && load_prg_rom() && load_chr_rom() &&
           load_play_choice_rom_() && load_play_choice_prom();
}