#include "bus.h"
#include <utility>

auto bus::cpu_bus_write(uint16_t addr, uint8_t data) -> void {
    if (addr <= 0x1FFF) {
        ram_[addr & 0x7ff] = data;
    } else if (addr <= 0x1FFF) {
        ppu_.cpu_bus_write(addr, data);
    }
}

auto bus::cpu_bus_read(uint16_t addr) -> uint8_t {
    if (addr <= 0x1FFF) {
        return ram_[addr & 0x7ff];
    } else if (addr <= 0x3FFF) {
        return ppu_.cpu_bus_read(addr);
    }
    std::unreachable();
}