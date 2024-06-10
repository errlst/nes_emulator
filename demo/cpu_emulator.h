#pragma once

#include "../3rd/imgui/imgui.h"
#include "../nes/bus.h"
#include <format>
#include <iostream>

class cpu_emulator {
  public:
    cpu_emulator() {
        static auto loaded = false;
        ImGui::Begin("cpu emulator");
        if (show_input_bincode()) {
            loaded = true;
            reload_strs();
            cpu_.reset();
        }
        if (loaded) {
            show_ram();
            ImGui::SameLine();
            ImGui::BeginChild("reg_and_instruction", {0, 400}, true);
            show_reg();
            show_inst();
            ImGui::EndChild();
            if (ImGui::Button("next inst")) {
                cpu_.next_inst();
                reload_strs();
            }
            ImGui::SameLine();
            if (ImGui::Button("reset")) {
                cpu_.reset();
                reload_strs();
            }
        }
        ImGui::End();
    }

  private:
    // 输入二进制代码，返回 true 表示开始加载
    static auto show_input_bincode() -> bool {
        static auto show_module = false;
        ImGui::Text("bincode");
        ImGui::InputTextMultiline("##bincode", hex_bincode, sizeof(hex_bincode));
        ImGui::SameLine();
        if (ImGui::Button("load bincode")) {
            if (!bincode_to_ram()) {
                show_module = true;
            } else {
                return true;
            }
        }
        if (show_module) {
            ImGui::OpenPopup("invalid hex");
            if (ImGui::BeginPopupModal("invalid hex", &show_module)) {
                ImGui::Text("invalid hex code");
                ImGui::EndPopup();
            }
        }
        return false;
    }

    // 将十六进制代码转换为二进制代码，并输入到 ram
    static auto bincode_to_ram() -> bool {
        for (auto i = 0, pos = 0; hex_bincode[i] != 0 && i < sizeof(hex_bincode);) {
            while (std::isspace(hex_bincode[i])) {
                ++i;
            }
            if (hex_bincode[i] == 0 || i >= sizeof(hex_bincode)) {
                return true;
            }
            if (valid_hex(hex_bincode[i]) && valid_hex(hex_bincode[i + 1])) {
                bus_.ram()[pos++] = (hex_to_int(hex_bincode[i]) << 4) | hex_to_int(hex_bincode[i + 1]);
                i += 2;
            } else {
                return false;
            }
        }
        return true;
    }

    // 内存信息
    static auto show_ram() -> void {
        ImGui::BeginChild("show_mem_info", {500, 400}, true);
        ImGui::Text("memory");
        ImGui::InputTextMultiline("##memory", ram_str.data(), ram_str.size(),
                                  {500, 300}, ImGuiInputTextFlags_ReadOnly);
        if (ImGui::InputInt("start address", &ram_str_start, 16, 0, ImGuiInputTextFlags_CharsHexadecimal)) {
            read_ram_str();
        }
        ImGui::EndChild();
    }

    // 寄存器信息
    static auto show_reg() -> void {
        ImGui::BeginChild("show_reg_info", {0, 150}, true);
        ImGui::Text("register");
        ImGui::InputTextMultiline("##register", reg_str.data(), reg_str.size(), {}, ImGuiInputTextFlags_ReadOnly);
        ImGui::EndChild();
    }

    // 指令信息
    static auto show_inst() -> void {
        static auto cur_pc = cpu_.pc();
        ImGui::BeginChild("show_instruction_info", {300, 200}, true);
        ImGui::Text("instruction");
        ImGui::InputTextMultiline("##instruction", inst_str.data(), inst_str.size(), {300, 150}, ImGuiInputTextFlags_ReadOnly);
        ImGui::EndChild();
    }

    // 重新加载数据
    static auto reload_strs() -> void {
        read_ram_str();
        read_reg_str();
        read_inst_str();
    }

    // 读取内存数据，读取20行
    static auto read_ram_str() -> void {
        ram_str.clear();
        for (auto i = 0; i < 20; ++i) {
            ram_str += std::format("${:04x}:  ", ram_str_start + i * 16);
            for (auto j = 0; j < 16; ++j) {
                ram_str += std::format("{:02x} ", bus_.ram()[ram_str_start + i * 16 + j]);
            }
            ram_str += '\n';
        }
    }

    // 读取寄存器状态
    static auto read_reg_str() -> void {
        reg_str = std::format("STATUS: N:{} V:{} U:- B:{} D:{} I:{} Z:{} C:{}\nPC: ${:04x}\nA:  ${:02x}\nX:  ${:02x}\nY:  ${:02x}\nSP: ${:02x}\n\n",
                              cpu_.stat().N, cpu_.stat().V, cpu_.stat().B, cpu_.stat().D, cpu_.stat().I, cpu_.stat().Z, cpu_.stat().C,
                              cpu_.pc(), cpu_.a(), cpu_.x(), cpu_.y(), cpu_.sp());
    }

    // 读取指令，从当前指令开始，往后读取 10 条指令
    static auto read_inst_str() -> void {
        inst_str.clear();
        for (int i = 0, pc = cpu_.pc(); i < 10; ++i) {
            inst_str += "    " + cpu::inst_str(bus_.ram(), pc);
            pc += cpu::inst_len(cpu::inst_table[bus_.ram()[pc]]);
        }
        inst_str[0] = '-';
        inst_str[1] = '>';
    }

    static auto valid_hex(char c) -> bool {
        return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
    }

    static auto hex_to_int(char c) -> int {
        if (c >= '0' && c <= '9') {
            return c - '0';
        } else if (c >= 'a' && c <= 'f') {
            return c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            return c - 'A' + 10;
        }
        return 0;
    }

  private:
    static inline bus bus_{};
    static inline cpu cpu_{bus_};
    static inline char hex_bincode[bus::ram_size * 2]{};
    static inline int ram_str_start;
    static inline std::string ram_str;  // 内存字符串
    static inline std::string reg_str;  // 寄存器字符串
    static inline std::string inst_str; // 指令字符串
};
