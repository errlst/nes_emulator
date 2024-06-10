#pragma once

#include "cpu.h"
#include <array>
#include <cstdint>

class bus {
  public:
    bus() : cpu_{*this} {
    }

    ~bus() = default;

    auto write(uint16_t addr, uint8_t data) -> void { ram_[addr] = data; }

    auto read(uint16_t addr) -> uint8_t { return ram_[addr]; }

    auto ram() -> uint8_t * { return ram_.data(); }

  public:
    constexpr static auto ram_size = 64 * 1024;

  private:
    cpu cpu_;
    std::array<uint8_t, ram_size> ram_{};
};
