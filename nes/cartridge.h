#pragma once
#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

// 卡带头部
struct cart_header {
    char identify[4];
    uint8_t prg_rom_size;
    uint8_t chr_rom_size;
    uint8_t flag_6;
    uint8_t flag_7;
    uint8_t prg_ram_size;
    uint8_t unused[7];
};
static_assert(sizeof(cart_header) == 16);

class cartridge {
  public:
    cartridge(const std::string &filename);

    auto valid() -> bool { return valid_; }

    // 加载卡带
  private:
    auto load() -> bool;
    auto load_header() -> bool;
    auto load_trainer() -> bool;
    auto load_rom() -> bool;
    auto load_mapper() -> bool;

    // 卡带内部存储
  private:
    cart_header header_;
    std::vector<uint8_t> trainer_;
    std::vector<uint8_t> prg_rom_;
    std::vector<uint8_t> chr_rom_;

  private:
    std::ifstream ifs_;
    bool valid_{};
};