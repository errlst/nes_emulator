#include "bus.h"
#include <format>
#include <utility>

auto cpu::read(uint16_t addr) -> uint8_t {
    return bus_.cpu_bus_read(addr);
}

auto cpu::write(uint16_t addr, uint8_t data) -> void {
    bus_.cpu_bus_write(addr, data);
}

auto cpu::next_clock() -> void {
    if (cycles_ == 0) {
        opcode_ = next_pc();
        cycles_ = inst_table[opcode_].cycles;
        (this->*inst_table[opcode_].mod)();
        (this->*inst_table[opcode_].opt)();
    }
    --cycles_;
}

auto cpu::next_inst() -> void {
    opcode_ = next_pc();
    (this->*inst_table[opcode_].mod)();
    (this->*inst_table[opcode_].opt)();
}

auto cpu::reset() -> void {
    r_a_ = 0;
    r_x_ = 0;
    r_y_ = 0;
    r_sp_ = 0xfd;
    *reinterpret_cast<uint8_t *>(&r_stat_) = 0;
    r_pc_ = (read(0xfffd) << 8) | read(0xfffc);
    addr_ = 0;
    off_ = 0;
    fetched_ = 0;
    cycles_ = 0;
}

auto cpu::irq() -> void {
    if (r_stat_.I == 0) {
        push_pc();
        push_stat();
        r_stat_.B = 0;
        r_stat_.I = 1;
        r_pc_ = read(0xfffe) | (read(0xffff) << 8);
        cycles_ = 7;
    }
}

auto cpu::nmi() -> void {
    push_pc();
    push_stat();
    r_stat_.B = 0;
    r_stat_.I = 1;
    r_pc_ = read(0xfffa) | (read(0xfffb) << 8);
    cycles_ = 8;
}

auto cpu::fetch() -> uint8_t {
    if (inst_table[opcode_].mod == &cpu::ACC) {
        return r_a_;
    }
    return fetched_ = read(addr_);
}

auto cpu::branch_if(bool cond) -> void {
    if (cond) {
        cycles_ += 1;
        r_pc_ += off_;
    }
}

auto cpu::push_pc() -> void {
    stack_push((r_pc_ >> 8) & 0xff);
    stack_push(r_pc_ & 0xff);
}

auto cpu::ABSX() -> void {
    auto lo = next_pc();
    auto hi = next_pc();
    addr_ = (lo | (hi << 8)) + r_x_;
}

auto cpu::ABSY() -> void {
    auto lo = read(r_pc_++);
    auto hi = read(r_pc_++);
    addr_ = (lo | (hi << 8)) + r_y_;
}

auto cpu::IND() -> void {
    addr_ = next_pc() | (next_pc() << 8);
    addr_ = read(addr_) | (read(addr_ + 1) << 8);
}

auto cpu::INDX() -> void {
    addr_ = (next_pc() + r_x_) & 0xff;
    addr_ = read(addr_) | (read(addr_ + 1) << 8);
}

auto cpu::INDY() -> void {
    addr_ = next_pc();
    addr_ = read(addr_) | (read(addr_ + 1) << 8) + r_y_;
}

auto cpu::ADC() -> void {
    const uint16_t tmp = static_cast<uint16_t>(r_a_) + fetch() + r_stat_.C;
    r_stat_.N = tmp & 0x80;
    r_stat_.V = ~(static_cast<uint16_t>(r_a_) ^ fetched_) & ((r_a_ ^ tmp) & 0x80);
    r_stat_.Z = r_a_ == 0;
    r_stat_.C = tmp > 255;
    r_a_ = tmp & 0xff;
}

auto cpu::AND() -> void {
    r_a_ &= fetch();
    r_stat_.N = r_a_ & 0x80;
    r_stat_.Z = r_a_ == 0;
}

auto cpu::ASL() -> void {
    const uint16_t tmp = fetch() << 1;
    r_stat_.N = tmp & 0x80;
    r_stat_.Z = (tmp & 0xff) == 0;
    r_stat_.C = tmp & 0xff00;
    if (inst_table[opcode_].mod == &cpu::ACC) {
        r_a_ = tmp & 0xff;
    } else {
        write(addr_, tmp & 0xff);
    }
}

auto cpu::BIT() -> void {
    r_stat_.N = (fetch() >> 7) & 0x1;
    r_stat_.V = (fetched_ >> 6) & 0x1;
    r_stat_.Z = (fetched_ & r_a_) == 0;
}

auto cpu::BRK() -> void {
    stack_push(r_pc_);
    r_pc_ = 0xfffe;
    r_stat_.B = 1;
}

auto cpu::CMP() -> void {
    const uint16_t tmp = static_cast<uint16_t>(r_a_) - fetch();
    r_stat_.N = tmp & 0x80;
    r_stat_.Z = tmp == 0;
    r_stat_.C = r_a_ >= fetched_;
}

auto cpu::CPX() -> void {
    const uint16_t tmp = static_cast<uint16_t>(r_x_) - fetch();
    r_stat_.N = tmp & 0x80;
    r_stat_.Z = tmp == 0;
    r_stat_.C = r_x_ >= fetched_;
}

auto cpu::CPY() -> void {
    const uint16_t tmp = static_cast<uint16_t>(r_y_) - fetch();
    r_stat_.N = tmp & 0x80;
    r_stat_.Z = tmp == 0;
    r_stat_.C = r_y_ >= fetched_;
}

auto cpu::DEC() -> void {
    const uint8_t tmp = fetch() - 1;
    r_stat_.N = tmp & 0x80;
    r_stat_.Z = tmp == 0;
    write(addr_, tmp & 0xff);
}

auto cpu::DEX() -> void {
    const uint8_t tmp = r_x_ - 1;
    r_stat_.N = tmp & 0x80;
    r_stat_.Z = tmp == 0;
    write(addr_, tmp & 0xff);
}

auto cpu::DEY() -> void {
    const uint8_t tmp = r_y_ - 1;
    r_stat_.N = tmp & 0x80;
    r_stat_.Z = tmp == 0;
    write(addr_, tmp & 0xff);
}

auto cpu::EOR() -> void {
    r_a_ ^= fetch();
    r_stat_.N = r_a_ & 0x80;
    r_stat_.Z = r_a_ == 0;
}

auto cpu::INC() -> void {
    const uint8_t tmp = fetch() + 1;
    r_stat_.N = tmp & 0x80;
    r_stat_.Z = tmp == 0;
    write(addr_, tmp);
}

auto cpu::INX() -> void {
    r_x_ += 1;
    r_stat_.N = r_x_ & 0x80;
    r_stat_.Z = r_x_ == 0;
}

auto cpu::INY() -> void {
    r_y_ += 1;
    r_stat_.N = r_y_ & 0x80;
    r_stat_.Z = r_y_ == 0;
}

auto cpu::JSR() -> void {
    push_pc();
    r_pc_ = addr_;
}

auto cpu::LDA() -> void {
    r_a_ = fetch();
    r_stat_.N = r_a_ & 0x80;
    r_stat_.Z = r_a_ == 0;
}

auto cpu::LDX() -> void {
    r_x_ = fetch();
    r_stat_.N = r_x_ & 0x80;
    r_stat_.Z = r_x_ == 0;
}

auto cpu::LDY() -> void {
    r_y_ = fetch();
    r_stat_.N = r_y_ & 0x80;
    r_stat_.Z = r_y_ == 0;
}

auto cpu::LSR() -> void {
    uint16_t tmp = fetch();
    r_stat_.C = tmp & 0x1;
    tmp >>= 1;
    r_stat_.N = tmp & 0x80;
    r_stat_.Z = tmp == 0;
    if (inst_table[opcode_].mod == &cpu::ACC) {
        r_a_ = tmp & 0xff;
    } else {
        write(addr_, tmp & 0xff);
    }
}

auto cpu::ORA() -> void {
    r_a_ |= fetch();
    r_stat_.N = r_a_ & 0x80;
    r_stat_.Z = r_a_ == 0;
}

auto cpu::PLA() -> void {
    r_a_ = stack_pull();
    r_stat_.N = r_a_ & 0x80;
    r_stat_.Z = r_a_ == 0;
}

auto cpu::ROL() -> void {
    const uint16_t tmp = (fetch() << 1) + r_stat_.C;
    r_stat_.N = tmp & 0x80;
    r_stat_.Z = tmp == 0;
    r_stat_.C = tmp & 0xff00;
    if (inst_table[opcode_].mod == &cpu::ACC) {
        r_a_ = tmp;
    } else {
        write(addr_, tmp);
    }
}

auto cpu::ROR() -> void {
    const uint16_t tmp = (fetch() >> 1) | (r_stat_.C << 7);
    r_stat_.N = tmp & 0x80;
    r_stat_.Z = tmp == 0;
    r_stat_.C = tmp & 0x1;
    if (inst_table[opcode_].mod == &cpu::ACC) {
        r_a_ = tmp;
    } else {
        write(addr_, tmp);
    }
}

auto cpu::RTI() -> void {
    pull_stat();
    r_stat_.B = ~r_stat_.B;
    r_stat_.I = ~r_stat_.I;
    pull_pc();
}

auto cpu::SBC() -> void {
    const uint16_t val = fetch() ^ 0xff;
    const uint16_t tmp = r_a_ + val + r_stat_.C;
    r_stat_.N = tmp & 0x80;
    r_stat_.V = (tmp ^ r_a_) & ((tmp ^ val) & 0x80);
    r_stat_.Z = (tmp & 0xff) == 0;
    r_stat_.C = tmp & 0xff00;
    r_a_ = tmp & 0xff;
}

auto cpu::TAX() -> void {
    r_x_ = r_a_;
    r_stat_.N = r_x_ & 0x80;
    r_stat_.Z = r_x_ == 0;
}

auto cpu::TAY() -> void {
    r_y_ = r_a_;
    r_stat_.N = r_y_ & 0x80;
    r_stat_.Z = r_y_ == 0;
}

auto cpu::TSX() -> void {
    r_x_ = r_sp_;
    r_stat_.N = r_x_ & 0x80;
    r_stat_.Z = r_x_ == 0;
}

auto cpu::TXA() -> void {
    r_a_ = r_x_;
    r_stat_.N = r_a_ & 0x80;
    r_stat_.Z = r_a_ == 0;
}

auto cpu::TYA() -> void {
    r_a_ = r_y_;
    r_stat_.N = r_a_ & 0x80;
    r_stat_.Z = r_a_ == 0;
}

auto cpu::inst_len(const instruction &inst) -> int {
    if (inst.mod == &cpu::ABS || inst.mod == &cpu::ABSX || inst.mod == &cpu::ABSY) {
        return 3;
    } else if (inst.mod == &cpu::ACC) {
        return 1;
    } else if (inst.mod == &cpu::IMM) {
        return 2;
    } else if (inst.mod == &cpu::IMP) {
        return 1;
    } else if (inst.mod == &cpu::IND) {
        return 3;
    } else if (inst.mod == &cpu::INDX || inst.mod == &cpu::INDY) {
        return 2;
    } else if (inst.mod == &cpu::REL) {
        return 2;
    } else if (inst.mod == &cpu::ZP || inst.mod == &cpu::ZPX || inst.mod == &cpu::ZPY) {
        return 2;
    }
    std::unreachable();
    abort();
}

// 格式：指令名称 $地址 | #立即数 | $[$地址]
auto cpu::inst_str(uint8_t *mem, uint16_t pc) -> std::string {
    auto inst = inst_table[mem[pc]];
    auto res = std::string{};
    if (inst.mod == &cpu::ABS) {
        res = std::format("${:04x}: {} ${:04x} ({}) {}c\n", pc, inst.name, (mem[pc + 1] | (mem[pc + 2] << 8)), "ABS", inst.cycles);
    } else if (inst.mod == &cpu::ABSX) {
        res = std::format("${:04x}: {} ${:04x}+x ({}) {}c\n", pc, inst.name, (mem[pc + 1] | (mem[pc + 2] << 8)), "ABSX", inst.cycles);
    } else if (inst.mod == &cpu::ABSY) {
        res = std::format("${:04x}: {} ${:04x}+y ({}) {}c\n", pc, inst.name, (mem[pc + 1] | (mem[pc + 2] << 8)), "ABSY", inst.cycles);
    } else if (inst.mod == &cpu::ACC) {
        res = std::format("${:04x}: {} ({}) {}c\n", pc, inst.name, "ACC", inst.cycles);
    } else if (inst.mod == &cpu::IMM) {
        res = std::format("${:04x}: {} #{:02x} ({}) {}c\n", pc, inst.name, mem[pc + 1], "IMM", inst.cycles);
    } else if (inst.mod == &cpu::IMP) {
        res = std::format("${:04x}: {} ({}) {}c\n", pc, inst.name, "IMP", inst.cycles);
    } else if (inst.mod == &cpu::IND) {
        res = std::format("${:04x}: {} $[${:02x}+x] ({}) {}c\n", pc, inst.name, mem[pc + 1], "IMP", inst.cycles);
    } else if (inst.mod == &cpu::INDX) {
        res = std::format("${:04x}: {} $[${:02x}+x] ({}) {}c\n", pc, inst.name, mem[pc + 1], "INDX", inst.cycles);
    } else if (inst.mod == &cpu::INDY) {
        res = std::format("${:04x}: {} $[${:02x}]+y ({}) {}c\n", pc, inst.name, mem[pc + 1], "INDY", inst.cycles);
    } else if (inst.mod == &cpu::REL) {
        res = std::format("${:04x}: {} ${:02x} ({}) {}c\n", pc, inst.name, mem[pc + 1], "REL", inst.cycles);
    } else if (inst.mod == &cpu::ZP) {
        res = std::format("${:04x}: {} ${:02x} ({}) {}c\n", pc, inst.name, mem[pc + 1], "ZP", inst.cycles);
    } else if (inst.mod == &cpu::ZPX) {
        res = std::format("${:04x}: {} ${:02x}+x ({}) {}c\n", pc, inst.name, mem[pc + 1], "ZPX", inst.cycles);
    } else if (inst.mod == &cpu::ZPY) {
        res = std::format("${:04x}: {} ${:02x}+y ({}) {}c\n", pc, inst.name, mem[pc + 1], "ZPY", inst.cycles);
    } else {
        std::unreachable();
        abort();
    }
    return res;
}

const std::vector<instruction> cpu::inst_table = {
    {"BRK", &cpu::BRK, &cpu::IMP, 7},
    {"ORA", &cpu::ORA, &cpu::INDX, 6},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 8},
    {"???", &cpu::NOP, &cpu::ACC, 3},
    {"ORA", &cpu::ORA, &cpu::ZP, 3},
    {"ASL", &cpu::ASL, &cpu::ZP, 5},
    {"???", &cpu::UNK, &cpu::ACC, 5},
    {"PHP", &cpu::PHP, &cpu::IMP, 3},
    {"ORA", &cpu::ORA, &cpu::IMM, 2},
    {"ASL", &cpu::ASL, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"???", &cpu::NOP, &cpu::ACC, 4},
    {"ORA", &cpu::ORA, &cpu::ABS, 4},
    {"ASL", &cpu::ASL, &cpu::ABS, 6},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"BPL", &cpu::BPL, &cpu::REL, 2},
    {"ORA", &cpu::ORA, &cpu::INDY, 5},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 8},
    {"???", &cpu::NOP, &cpu::ACC, 4},
    {"ORA", &cpu::ORA, &cpu::ZPX, 4},
    {"ASL", &cpu::ASL, &cpu::ZPX, 6},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"CLC", &cpu::CLC, &cpu::IMP, 2},
    {"ORA", &cpu::ORA, &cpu::ABSY, 4},
    {"???", &cpu::NOP, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 7},
    {"???", &cpu::NOP, &cpu::ACC, 4},
    {"ORA", &cpu::ORA, &cpu::ABSX, 4},
    {"ASL", &cpu::ASL, &cpu::ABSX, 7},
    {"???", &cpu::UNK, &cpu::ACC, 7},
    {"JSR", &cpu::JSR, &cpu::ABS, 6},
    {"AND", &cpu::AND, &cpu::INDX, 6},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 8},
    {"BIT", &cpu::BIT, &cpu::ZP, 3},
    {"AND", &cpu::AND, &cpu::ZP, 3},
    {"ROL", &cpu::ROL, &cpu::ZP, 5},
    {"???", &cpu::UNK, &cpu::ACC, 5},
    {"PLP", &cpu::PLP, &cpu::IMP, 4},
    {"AND", &cpu::AND, &cpu::IMM, 2},
    {"ROL", &cpu::ROL, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"BIT", &cpu::BIT, &cpu::ABS, 4},
    {"AND", &cpu::AND, &cpu::ABS, 4},
    {"ROL", &cpu::ROL, &cpu::ABS, 6},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"BMI", &cpu::BMI, &cpu::REL, 2},
    {"AND", &cpu::AND, &cpu::INDY, 5},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 8},
    {"???", &cpu::NOP, &cpu::ACC, 4},
    {"AND", &cpu::AND, &cpu::ZPX, 4},
    {"ROL", &cpu::ROL, &cpu::ZPX, 6},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"SEC", &cpu::SEC, &cpu::IMP, 2},
    {"AND", &cpu::AND, &cpu::ABSY, 4},
    {"???", &cpu::NOP, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 7},
    {"???", &cpu::NOP, &cpu::ACC, 4},
    {"AND", &cpu::AND, &cpu::ABSX, 4},
    {"ROL", &cpu::ROL, &cpu::ABSX, 7},
    {"???", &cpu::UNK, &cpu::ACC, 7},
    {"RTI", &cpu::RTI, &cpu::IMP, 6},
    {"EOR", &cpu::EOR, &cpu::INDX, 6},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 8},
    {"???", &cpu::NOP, &cpu::ACC, 3},
    {"EOR", &cpu::EOR, &cpu::ZP, 3},
    {"LSR", &cpu::LSR, &cpu::ZP, 5},
    {"???", &cpu::UNK, &cpu::ACC, 5},
    {"PHA", &cpu::PHA, &cpu::IMP, 3},
    {"EOR", &cpu::EOR, &cpu::IMM, 2},
    {"LSR", &cpu::LSR, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"JMP", &cpu::JMP, &cpu::ABS, 3},
    {"EOR", &cpu::EOR, &cpu::ABS, 4},
    {"LSR", &cpu::LSR, &cpu::ABS, 6},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"BVC", &cpu::BVC, &cpu::REL, 2},
    {"EOR", &cpu::EOR, &cpu::INDY, 5},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 8},
    {"???", &cpu::NOP, &cpu::ACC, 4},
    {"EOR", &cpu::EOR, &cpu::ZPX, 4},
    {"LSR", &cpu::LSR, &cpu::ZPX, 6},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"CLI", &cpu::CLI, &cpu::IMP, 2},
    {"EOR", &cpu::EOR, &cpu::ABSY, 4},
    {"???", &cpu::NOP, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 7},
    {"???", &cpu::NOP, &cpu::ACC, 4},
    {"EOR", &cpu::EOR, &cpu::ABSX, 4},
    {"LSR", &cpu::LSR, &cpu::ABSX, 7},
    {"???", &cpu::UNK, &cpu::ACC, 7},
    {"RTS", &cpu::RTS, &cpu::IMP, 6},
    {"ADC", &cpu::ADC, &cpu::INDX, 6},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 8},
    {"???", &cpu::NOP, &cpu::ACC, 3},
    {"ADC", &cpu::ADC, &cpu::ZP, 3},
    {"ROR", &cpu::ROR, &cpu::ZP, 5},
    {"???", &cpu::UNK, &cpu::ACC, 5},
    {"PLA", &cpu::PLA, &cpu::IMP, 4},
    {"ADC", &cpu::ADC, &cpu::IMM, 2},
    {"ROR", &cpu::ROR, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"JMP", &cpu::JMP, &cpu::IND, 5},
    {"ADC", &cpu::ADC, &cpu::ABS, 4},
    {"ROR", &cpu::ROR, &cpu::IMP, 6},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"BVS", &cpu::BVS, &cpu::REL, 2},
    {"ADC", &cpu::ADC, &cpu::INDY, 5},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 8},
    {"???", &cpu::NOP, &cpu::ACC, 4},
    {"ADC", &cpu::ADC, &cpu::ZPX, 4},
    {"ROR", &cpu::ROR, &cpu::ZPX, 6},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"SEI", &cpu::SEI, &cpu::IMP, 2},
    {"ADC", &cpu::ADC, &cpu::ABSY, 4},
    {"???", &cpu::NOP, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 7},
    {"???", &cpu::NOP, &cpu::ACC, 4},
    {"ADC", &cpu::ADC, &cpu::ABSX, 4},
    {"ROR", &cpu::ROR, &cpu::ABSX, 7},
    {"???", &cpu::UNK, &cpu::ACC, 7},
    {"???", &cpu::NOP, &cpu::ACC, 2},
    {"STA", &cpu::STA, &cpu::INDX, 6},
    {"???", &cpu::NOP, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"STY", &cpu::STY, &cpu::ZP, 3},
    {"STA", &cpu::STA, &cpu::ZP, 3},
    {"STX", &cpu::STX, &cpu::ZP, 3},
    {"???", &cpu::UNK, &cpu::ACC, 3},
    {"DEY", &cpu::DEY, &cpu::IMP, 2},
    {"???", &cpu::NOP, &cpu::ACC, 2},
    {"TXA", &cpu::TXA, &cpu::IMP, 2},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"STY", &cpu::STY, &cpu::ABS, 4},
    {"STA", &cpu::STA, &cpu::ABS, 4},
    {"STX", &cpu::STX, &cpu::ABS, 4},
    {"???", &cpu::UNK, &cpu::ACC, 4},
    {"BCC", &cpu::BCC, &cpu::REL, 2},
    {"STA", &cpu::STA, &cpu::INDY, 6},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"STY", &cpu::STY, &cpu::ZPX, 4},
    {"STA", &cpu::STA, &cpu::ZPX, 4},
    {"STX", &cpu::STX, &cpu::ZPY, 4},
    {"???", &cpu::UNK, &cpu::ACC, 4},
    {"TYA", &cpu::TYA, &cpu::IMP, 2},
    {"STA", &cpu::STA, &cpu::ABSY, 5},
    {"TXS", &cpu::TXS, &cpu::IMP, 2},
    {"???", &cpu::UNK, &cpu::ACC, 5},
    {"???", &cpu::NOP, &cpu::ACC, 5},
    {"STA", &cpu::STA, &cpu::ABSX, 5},
    {"???", &cpu::UNK, &cpu::ACC, 5},
    {"???", &cpu::UNK, &cpu::ACC, 5},
    {"LDY", &cpu::LDY, &cpu::IMM, 2},
    {"LDA", &cpu::LDA, &cpu::INDX, 6},
    {"LDX", &cpu::LDX, &cpu::IMM, 2},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"LDY", &cpu::LDY, &cpu::ZP, 3},
    {"LDA", &cpu::LDA, &cpu::ZP, 3},
    {"LDX", &cpu::LDX, &cpu::ZP, 3},
    {"???", &cpu::UNK, &cpu::ACC, 3},
    {"TAY", &cpu::TAY, &cpu::IMP, 2},
    {"LDA", &cpu::LDA, &cpu::IMM, 2},
    {"TAX", &cpu::TAX, &cpu::IMP, 2},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"LDY", &cpu::LDY, &cpu::ABS, 4},
    {"LDA", &cpu::LDA, &cpu::ABS, 4},
    {"LDX", &cpu::LDX, &cpu::ABS, 4},
    {"???", &cpu::UNK, &cpu::ACC, 4},
    {"BCS", &cpu::BCS, &cpu::REL, 2},
    {"LDA", &cpu::LDA, &cpu::INDY, 5},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 5},
    {"LDY", &cpu::LDY, &cpu::ZPX, 4},
    {"LDA", &cpu::LDA, &cpu::ZPX, 4},
    {"LDX", &cpu::LDX, &cpu::ZPY, 4},
    {"???", &cpu::UNK, &cpu::ACC, 4},
    {"CLV", &cpu::CLV, &cpu::IMP, 2},
    {"LDA", &cpu::LDA, &cpu::ABSY, 4},
    {"TSX", &cpu::TSX, &cpu::IMP, 2},
    {"???", &cpu::UNK, &cpu::ACC, 4},
    {"LDY", &cpu::LDY, &cpu::ABSX, 4},
    {"LDA", &cpu::LDA, &cpu::ABSX, 4},
    {"LDX", &cpu::LDX, &cpu::ABSY, 4},
    {"???", &cpu::UNK, &cpu::ACC, 4},
    {"CPY", &cpu::CPY, &cpu::IMM, 2},
    {"CMP", &cpu::CMP, &cpu::INDX, 6},
    {"???", &cpu::NOP, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 8},
    {"CPY", &cpu::CPY, &cpu::ZP, 3},
    {"CMP", &cpu::CMP, &cpu::ZP, 3},
    {"DEC", &cpu::DEC, &cpu::ZP, 5},
    {"???", &cpu::UNK, &cpu::ACC, 5},
    {"INY", &cpu::INY, &cpu::IMP, 2},
    {"CMP", &cpu::CMP, &cpu::IMM, 2},
    {"DEX", &cpu::DEX, &cpu::IMP, 2},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"CPY", &cpu::CPY, &cpu::ABS, 4},
    {"CMP", &cpu::CMP, &cpu::ABS, 4},
    {"DEC", &cpu::DEC, &cpu::ABS, 6},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"BNE", &cpu::BNE, &cpu::REL, 2},
    {"CMP", &cpu::CMP, &cpu::INDY, 5},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 8},
    {"???", &cpu::NOP, &cpu::ACC, 4},
    {"CMP", &cpu::CMP, &cpu::ZPX, 4},
    {"DEC", &cpu::DEC, &cpu::ZPX, 6},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"CLD", &cpu::CLD, &cpu::IMP, 2},
    {"CMP", &cpu::CMP, &cpu::ABSY, 4},
    {"NOP", &cpu::NOP, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 7},
    {"???", &cpu::NOP, &cpu::ACC, 4},
    {"CMP", &cpu::CMP, &cpu::ABSX, 4},
    {"DEC", &cpu::DEC, &cpu::ABSX, 7},
    {"???", &cpu::UNK, &cpu::ACC, 7},
    {"CPX", &cpu::CPX, &cpu::IMM, 2},
    {"SBC", &cpu::SBC, &cpu::INDX, 6},
    {"???", &cpu::NOP, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 8},
    {"CPX", &cpu::CPX, &cpu::ZP, 3},
    {"SBC", &cpu::SBC, &cpu::ZP, 3},
    {"INC", &cpu::INC, &cpu::ZP, 5},
    {"???", &cpu::UNK, &cpu::ACC, 5},
    {"INX", &cpu::INX, &cpu::IMP, 2},
    {"SBC", &cpu::SBC, &cpu::IMM, 2},
    {"NOP", &cpu::NOP, &cpu::IMP, 2},
    {"???", &cpu::SBC, &cpu::ACC, 2},
    {"CPX", &cpu::CPX, &cpu::ABS, 4},
    {"SBC", &cpu::SBC, &cpu::ABS, 4},
    {"INC", &cpu::INC, &cpu::ABS, 6},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"BEQ", &cpu::BEQ, &cpu::REL, 2},
    {"SBC", &cpu::SBC, &cpu::INDY, 5},
    {"???", &cpu::UNK, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 8},
    {"???", &cpu::NOP, &cpu::ACC, 4},
    {"SBC", &cpu::SBC, &cpu::ZPX, 4},
    {"INC", &cpu::INC, &cpu::ZPX, 6},
    {"???", &cpu::UNK, &cpu::ACC, 6},
    {"SED", &cpu::SED, &cpu::IMP, 2},
    {"SBC", &cpu::SBC, &cpu::ABSY, 4},
    {"NOP", &cpu::NOP, &cpu::ACC, 2},
    {"???", &cpu::UNK, &cpu::ACC, 7},
    {"???", &cpu::NOP, &cpu::ACC, 4},
    {"SBC", &cpu::SBC, &cpu::ABSX, 4},
    {"INC", &cpu::INC, &cpu::ABSX, 7},
    {"???", &cpu::UNK, &cpu::ACC, 7},
};
