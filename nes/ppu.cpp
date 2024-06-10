#include "bus.h"
#include <utility>

// control      0    w
// mask         1    w
// status       2    r
// oam addr     3    w
// oam data     4    rw
// scroll       5    w
// addr         6    w
// data         7    rw
auto ppu::cpu_bus_read(uint16_t addr) -> uint8_t {
    switch (addr & 0x7) {
        case 0:
        case 1:
            return 0;
        case 2:
            return *reinterpret_cast<uint8_t *>(&r_stat_);
        case 3:
            return 0;
        case 4:
            return reinterpret_cast<uint8_t *>(oam_)[oam_addr_];
        case 5:
        case 6:
            return 0;
        case 7:
            // todo))
    }
    std::unreachable();
}

auto ppu::cpu_bus_write(uint16_t addr, uint8_t data) -> void {
    switch (addr & 0x7) {
        case 0:
            *reinterpret_cast<uint8_t *>(&r_ctrl_) = data;
            break;
        case 1:
            *reinterpret_cast<uint8_t *>(&r_mask_) = data;
            break;
        case 2:
            break;
        case 3:
            oam_addr_ = data;
            break;
        case 4:
            reinterpret_cast<uint8_t *>(oam_)[oam_addr_] = data;
            break;
        case 5:
            r_scroll_ = data;
            break;
        case 6:
            r_vram_addr_ = data;
            break;
        case 7:
            // todo))
    }
}