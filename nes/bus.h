#pragma once
#include "cartridge.h"
#include "cpu.h"
#include "ppu.h"
#include <array>
#include <cstdint>
#include <memory>

class bus {
  public:
    bus() : cpu_{*this}, ppu_{*this} {
    }

  public:
    auto cpu_bus_write(uint16_t addr, uint8_t data) -> void;
    auto cpu_bus_read(uint16_t addr) -> uint8_t;

    auto ram() -> uint8_t * { return ram_.data(); }
    auto vram() -> uint8_t * { return vram_.data(); }

    // 卡带管理
  public:
    auto load_cartridget(std::shared_ptr<cartridge> cart) -> void { cart_ = cart; }
    auto cartridget() -> std::shared_ptr<cartridge> { return cart_; }

  private:
    cpu cpu_;
    ppu ppu_;
    std::shared_ptr<cartridge> cart_;
    std::array<uint8_t, 2 * 1024> ram_{};  // 2KB Ram
    std::array<uint8_t, 2 * 1024> vram_{}; // 2KB vRam
};
