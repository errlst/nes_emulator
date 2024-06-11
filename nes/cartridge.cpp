#include "cartridge.h"
#include <string_view>

cartridge::cartridge(const std::string &filename) {
    ifs_.open(filename, std::ifstream::binary);
    if (ifs_) {
        valid_ = load();
    }
}

auto cartridge::load() -> bool {
    return load_header() && load_trainer() && load_rom() && load_mapper();
}

auto cartridge::load_header() -> bool {
    ifs_.read(reinterpret_cast<char *>(&header_), sizeof(header_));
    ifs_.seekg(sizeof(header_), ifs_.cur);
    return std::string_view(header_.identify, 4) == "NES\x1A";
}

auto cartridge::load_trainer() -> bool {
    if (header_.flag_6 & 0x04) {
        trainer_.resize(512);
        ifs_.read(reinterpret_cast<char *>(trainer_.data()), trainer_.size());
        ifs_.seekg(512, ifs_.cur);
    }
    return true;
}

auto cartridge::load_rom() -> bool {
    if (((header_.flag_7 >> 2) & 0b11) != 1) { // 只支持 nes1.0
        return false;
    }

    prg_rom_.resize(header_.prg_rom_size * 16 * 1024);
    ifs_.read(reinterpret_cast<char *>(prg_rom_.data()), prg_rom_.size());
    ifs_.seekg(prg_rom_.size(), ifs_.cur);

    if (header_.chr_rom_size == 0) { // chr ram
        chr_rom_.resize(8192);
    } else {
        chr_rom_.resize(header_.chr_rom_size * 8 * 1024);
    }
    ifs_.read(reinterpret_cast<char *>(chr_rom_.size()), chr_rom_.size());
    return true;
}

auto cartridge::load_mapper() -> bool {
    switch ((header_.flag_6 >> 4) | ((header_.flag_7 >> 4) << 4)) {
        // todo))
    }

    return 0;
}