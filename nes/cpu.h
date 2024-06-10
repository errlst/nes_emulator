#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct instruction;
class bus;

// 状态寄存器
struct status_register {
    unsigned C : 1 {};
    unsigned Z : 1 {};
    unsigned I : 1 {};
    unsigned D : 1 {};
    unsigned B : 1 {};
    unsigned U : 1 {}; // 未使用
    unsigned V : 1 {};
    unsigned N : 1 {};
};

class cpu {
  public:
    using opt_type = void (cpu::*)();  // 指令操作
    using addr_mode = void (cpu::*)(); // 寻址模式

  public:
    explicit cpu(bus &b) : bus_(b) {}

    // 指令
  public:
    // 存取指令
    auto LDA() -> void; // a = fetched
    auto LDX() -> void; // x = fetched
    auto LDY() -> void; // y = fetched
    auto STA() -> void { write(addr_, r_a_); }
    auto STX() -> void { write(addr_, r_x_); }
    auto STY() -> void { write(addr_, r_y_); }

    // 堆栈指令
    auto PHA() -> void { stack_push(r_a_); } // push a
    auto PLA() -> void;                      // pull a
    auto PHP() -> void { push_stat(); }      // push stat
    auto PLP() -> void { pull_stat(); }      // pull stat

    // 传送指令
    auto TSX() -> void;                  // x = sp
    auto TXS() -> void { r_sp_ = r_x_; } // sp = x
    auto TAX() -> void;                  // x = a
    auto TXA() -> void;                  // a = x
    auto TAY() -> void;                  // y = a
    auto TYA() -> void;                  // a = y

    // 流程控制
    auto BMI() -> void { branch_if(r_stat_.N); }  // if (n) then branch
    auto BPL() -> void { branch_if(!r_stat_.N); } // if (!n) then branch
    auto BVS() -> void { branch_if(r_stat_.V); }  // if (v) then branch
    auto BVC() -> void { branch_if(!r_stat_.V); } // if (!v) then branch
    auto BEQ() -> void { branch_if(r_stat_.Z); }  // if (z) then branch
    auto BNE() -> void { branch_if(!r_stat_.Z); } // if (!z) then branch
    auto BCS() -> void { branch_if(r_stat_.C); }  // if (c) then branch
    auto BCC() -> void { branch_if(!r_stat_.C); } // if (!c) then branch
    auto JMP() -> void { r_pc_ = addr_; }         // 无条件跳转
    auto JSR() -> void;                           // 无条件跳转，但保存返回地址
    auto RTS() -> void { pull_pc(); }             // jsr 后返回
    auto RTI() -> void;                           // 中断后返回
    auto BRK() -> void;                           // 强行中断，并跳转到 0xfffe

    // 标志位控制
    auto CLV() -> void { r_stat_.V = 0; }
    auto CLD() -> void { r_stat_.D = 0; }
    auto SED() -> void { r_stat_.D = 1; }
    auto CLI() -> void { r_stat_.I = 0; }
    auto SEI() -> void { r_stat_.I = 1; }
    auto CLC() -> void { r_stat_.C = 0; }
    auto SEC() -> void { r_stat_.C = 1; }

    // 逻辑指令
    auto AND() -> void; // a &= fetched
    auto ORA() -> void; // a |= fetched
    auto EOR() -> void; // a ^= fetched
    auto BIT() -> void; //

    // 算数指令
    auto ADC() -> void; // a += fetched
    auto SBC() -> void; // a -= fetched

    // 比较指令
    auto CMP() -> void; // a - fetched
    auto CPX() -> void; // x - fetched
    auto CPY() -> void; // y - fetched

    // 位移指令
    auto ASL() -> void; // 算数左移一位
    auto LSR() -> void; // 逻辑右移一位
    auto ROL() -> void;
    auto ROR() -> void;

    // 递增、递减指令
    auto INC() -> void; // [mem]++
    auto DEC() -> void; // [mem]--
    auto INX() -> void; // x++
    auto DEX() -> void; // x--
    auto INY() -> void; // y++
    auto DEY() -> void; // y--

    // 其他指令
    auto NOP() -> void {}
    auto UNK() -> void {} // 未知指令

    // 寻址
  public:
    auto ABS() -> void { addr_ = next_pc() | (next_pc() << 8); }            // absolute              add = mem[pc] | (mem[pc+1] << 8)
    auto ABSX() -> void;                                                    // absolute x            add = abs + x
    auto ABSY() -> void;                                                    // absolute y            add = abs + y
    auto ACC() -> void {}                                                   // accumulator           none
    auto IMM() -> void { addr_ = r_pc_++; }                                 // immediate             add = pc
    auto IMP() -> void {}                                                   // implied               none
    auto IND() -> void;                                                     // absolute indirect     add = mem[abs] | (mem[abs+1] << 8)
    auto INDX() -> void;                                                    // indexed indirect x    add = mem[mem[pc] + x]
    auto INDY() -> void;                                                    // indirect indexed y    add = mem[mem[pc]] + y
    auto REL() -> void { *reinterpret_cast<uint8_t *>(&off_) = next_pc(); } // relative
    auto ZP() -> void { addr_ = next_pc(); }                                // zero page             add = mem[pc]
    auto ZPX() -> void { addr_ = (next_pc() + r_x_) & 0xff; }               // zero page x           add = mem[pc] + x
    auto ZPY() -> void { addr_ = (next_pc() + r_y_) & 0xff; }               // zero page y           add = mem[pc] + y

  public:
    auto next_clock() -> void; // 执行一次时钟周期
    auto next_inst() -> void;  // 执行一条指令
    auto reset() -> void;      // 重置
    auto irq() -> void;        // 中断
    auto nmi() -> void;        // 不可屏蔽中断

  private:
    auto read(uint16_t addr) -> uint8_t;
    auto write(uint16_t addr, uint8_t data) -> void;
    auto stack_push(uint8_t data) -> void { write(0x100 + (r_sp_--), data); }
    auto stack_pull() -> uint8_t { return read(0x100 + (++r_sp_)); }
    auto push_pc() -> void;
    auto pull_pc() -> void { r_pc_ = stack_pull() | (stack_pull() << 8); }
    auto push_stat() -> void { stack_push(*reinterpret_cast<uint8_t *>(&r_stat_)); }
    auto pull_stat() -> void { *reinterpret_cast<uint8_t *>(&r_stat_) = stack_pull(); }
    auto next_pc() -> uint8_t { return read(r_pc_++); }
    auto fetch() -> uint8_t;
    auto branch_if(bool cond) -> void;

    // 寄存器
  public:
    uint8_t r_a_{};
    uint8_t r_x_{};
    uint8_t r_y_{};
    uint8_t r_sp_{};
    uint16_t r_pc_{};
    status_register r_stat_;

    auto a() -> uint8_t { return r_a_; }
    auto x() -> uint8_t { return r_x_; }
    auto y() -> uint8_t { return r_y_; }
    auto sp() -> uint8_t { return r_sp_; }
    auto pc() -> uint16_t { return r_pc_; }
    auto stat() -> status_register { return r_stat_; }

    // 辅助函数
  public:
    static auto inst_len(const instruction &inst) -> int;           // 指令长度
    static auto inst_str(uint8_t *mem, uint16_t pc) -> std::string; // 指令字符串

  private:
    bus &bus_;
    uint32_t clocks_{}; // 时钟周期计数
    uint8_t cycles_{};  // 当前指令剩余执行周期
    uint8_t opcode_{};  // 当前指令
    uint8_t fetched_{}; // 读取的数据
    uint16_t addr_{};   // 地址
    int8_t off_{};      // 偏移

  public:
    const static std::vector<instruction> inst_table; // 指令表
};

struct instruction {
    std::string name;     // 助记符
    cpu::opt_type opt{};  // 操作
    cpu::addr_mode mod{}; // 寻址模式
    uint8_t cycles{};     // 执行周期
};
