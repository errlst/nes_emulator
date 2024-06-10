#pragma once
#include "cartridge.h"
#include <cstdint>

class bus;

// ctrl 寄存器
struct ppu_reg_ctrl {
    uint8_t name_table_idx : 2;
    uint8_t vram_inc : 1;
    uint8_t sprite_table_addr : 1;
    uint8_t bg_table_addr : 1;
    uint8_t sprite_size : 1;
    uint8_t master_slave : 1;
    uint8_t nmi : 1;
};
static_assert(sizeof(ppu_reg_ctrl) == 1);

// mask 寄存器
struct ppu_reg_mask {
    uint8_t greyscale : 1;
    uint8_t showbg_l : 1;
    uint8_t showsp_l : 1;
    uint8_t showbg : 1;
    uint8_t showsp : 1;
    uint8_t red_emph : 1;
    uint8_t green_emph : 1;
    uint8_t blue_emph : 1;
};
static_assert(sizeof(ppu_reg_mask) == 1);

// status 寄存器
struct ppu_reg_status {
    uint8_t unused : 5;
    uint8_t sp_overflow : 1;
    uint8_t sp_zero_hint : 1;
    uint8_t v_blank : 1;
};
static_assert(sizeof(ppu_reg_status) == 1);

// oam 条目
struct oam_entry {
    uint8_t y_pos;
    uint8_t tile_idx;
    uint8_t attr;
    uint8_t x_pos;
};

class ppu {
  public:
    ppu(bus &b) : bus_(b) {}

  public:
    auto cpu_bus_read(uint16_t addr) -> uint8_t;
    auto cpu_bus_write(uint16_t addr, uint8_t data) -> void;

    auto ppu_bus_read(uint16_t addr) -> uint8_t;
    auto ppu_bus_write(uint16_t addr) -> void;

    // 寄存器
  private:
    ppu_reg_ctrl r_ctrl_{};
    ppu_reg_mask r_mask_{};
    ppu_reg_status r_stat_{};
    uint8_t r_oam_addr_{};
    uint8_t r_oam_data_{};
    uint8_t r_scroll_{};
    uint8_t r_vram_addr_{};
    uint8_t r_vram_data_{};

    // 内部存储
  private:
    uint8_t palette_ram_idx_[32]{};
    uint8_t oam_addr_{};
    oam_entry oam_[64]{};

  private:
    bus &bus_;
};