#pragma once
#include <memory>
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
    explicit cpu(bus &b);

    ~cpu() = default;

    // 指令
  public:
    // 存取指令
    auto LDA() -> void; // a = fetched
    auto LDX() -> void; // x = fetched
    auto LDY() -> void; // y = fetched
    auto STA() -> void { write(addr_abs, r_a); }
    auto STX() -> void { write(addr_abs, r_x); }
    auto STY() -> void { write(addr_abs, r_y); }

    // 堆栈指令
    auto PHA() -> void { stack_push(r_a); } // push a
    auto PLA() -> void;                     // pull a
    auto PHP() -> void { push_stat(); }     // push stat
    auto PLP() -> void { pull_stat(); }     // pull stat

    // 传送指令
    auto TSX() -> void;                // x = sp
    auto TXS() -> void { r_sp = r_x; } // sp = x
    auto TAX() -> void;                // x = a
    auto TXA() -> void;                // a = x
    auto TAY() -> void;                // y = a
    auto TYA() -> void;                // a = y

    // 流程控制
    auto BMI() -> void { do_branch(r_stat.N); }  // if (n) then branch
    auto BPL() -> void { do_branch(!r_stat.N); } // if (!n) then branch
    auto BVS() -> void { do_branch(r_stat.V); }  // if (v) then branch
    auto BVC() -> void { do_branch(!r_stat.V); } // if (!v) then branch
    auto BEQ() -> void { do_branch(r_stat.Z); }  // if (z) then branch
    auto BNE() -> void { do_branch(!r_stat.Z); } // if (!z) then branch
    auto BCS() -> void { do_branch(r_stat.C); }  // if (c) then branch
    auto BCC() -> void { do_branch(!r_stat.C); } // if (!c) then branch
    auto JMP() -> void { r_pc = addr_abs; }      // 无条件跳转
    auto JSR() -> void;                          // 无条件跳转，但保存返回地址
    auto RTS() -> void;                          // jsr 后返回
    auto RTI() -> void;                          // 中断后返回
    auto BRK() -> void;                          // 强行中断，并跳转到 0xfffe

    // 标志位控制
    auto CLV() -> void { r_stat.V = 0; }
    auto CLD() -> void { r_stat.D = 0; }
    auto SED() -> void { r_stat.D = 1; }
    auto CLI() -> void { r_stat.I = 0; }
    auto SEI() -> void { r_stat.I = 1; }
    auto CLC() -> void { r_stat.C = 0; }
    auto SEC() -> void { r_stat.C = 1; }

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
    auto ABS() -> void { addr_abs = next_pc() | (next_pc() << 8); }             // absolute              add = mem[pc] | (mem[pc+1] << 8)
    auto ABSX() -> void;                                                        // absolute x            add = abs + x
    auto ABSY() -> void;                                                        // absolute y            add = abs + y
    auto ACC() -> void {}                                                       // accumulator           none
    auto IMM() -> void { addr_abs = r_pc++; }                                   // immediate             add = pc
    auto IMP() -> void {}                                                       // implied               none
    auto IND() -> void;                                                         // absolute indirect     add = mem[abs] | (mem[abs+1] << 8)
    auto INDX() -> void;                                                        // indexed indirect x    add = mem[mem[pc] + x]
    auto INDY() -> void;                                                        // indirect indexed y    add = mem[mem[pc]] + y
    auto REL() -> void { *reinterpret_cast<uint8_t *>(&addr_off) = next_pc(); } // relative
    auto ZP() -> void { addr_abs = next_pc(); }                                 // zero page             add = mem[pc]
    auto ZPX() -> void { addr_abs = (next_pc() + r_x) & 0xff; }                 // zero page x           add = mem[pc] + x
    auto ZPY() -> void { addr_abs = (next_pc() + r_y) & 0xff; }                 // zero page y           add = mem[pc] + y

  public:
    auto next_clock() -> void; // 执行一次时钟周期
    auto next_inst() -> void;  // 执行一条指令
    auto reset() -> void;      // 重置
    auto irq() -> void;        // 中断
    auto nmi() -> void;        // 不可屏蔽中断

  private:
    auto read(uint16_t addr) -> uint8_t;
    auto write(uint16_t addr, uint8_t data) -> void;
    auto stack_push(uint8_t data) -> void;
    auto stack_pull() -> uint8_t;
    auto next_pc() -> uint8_t;
    auto fetch() -> uint8_t;
    auto do_branch(bool branch) -> void; // 转移
    auto push_pc() -> void;              // 保存 pc
    auto pull_pc() -> void;              // 读取 pc
    auto push_stat() -> void;            // 保存 stat
    auto pull_stat() -> void;            // 读取 stat

    // 寄存器
  public:
    uint8_t r_a{};
    uint8_t r_x{};
    uint8_t r_y{};
    uint8_t r_sp{};
    uint16_t r_pc{};
    status_register r_stat;

    auto a() -> uint8_t { return r_a; }
    auto x() -> uint8_t { return r_x; }
    auto y() -> uint8_t { return r_y; }
    auto sp() -> uint8_t { return r_sp; }
    auto pc() -> uint16_t { return r_pc; }
    auto stat() -> status_register { return r_stat; }

    // 辅助函数
  public:
    static auto inst_len(const instruction &inst) -> int;           // 指令长度
    static auto inst_str(uint8_t *mem, uint16_t pc) -> std::string; // 指令字符串

  private:
    bus &bus_;
    uint32_t clock_count{}; // 时钟周期计数
    uint8_t cycles{};       // 当前指令剩余执行周期
    uint8_t opcode{};       // 当前指令
    uint8_t fetched{};      // 读取的数据
    int8_t addr_off{};      // 偏移
    uint16_t addr_abs{};    // 绝对地址
  public:
    const static std::vector<instruction> inst_table; // 指令表
};

struct instruction {
    std::string name;     // 助记符
    cpu::opt_type opt{};  // 操作
    cpu::addr_mode mod{}; // 寻址模式
    uint8_t cycles{};     // 执行周期
};
