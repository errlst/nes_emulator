bus 同时包含 cpu_bus 和 ppu_bus 的功能。

#### cpu内存
cpu 可以寻址 16 位内存。
| 地址范围        | 说明            |
| --------------- | --------------- |
| 0x0000 ~ 0x07FF | 2KB 内部 ram    |
| 0x0800 ~ 0x1fFF | ram 镜像（3层） |
| 0x2000 ~ 0x2007 | ppu 寄存器      |
| 0x2008 ~ 0x3FFF | ppu 寄存器 镜像 |
| 0x4000 ~ 0x4017 | apu & io 寄存器 |
| 0x4018 ~ 0x401F | 忽略            |
| 0x4020 ~ 0xFFFF | 卡带使用        |

参考：https://www.nesdev.org/wiki/CPU_memory_map


#### ppu 内存
ppu 可以寻址 14 位内存。
| 地址范围        | 说明                      |
| --------------- | ------------------------- |
| 0x0000 ~ 0x0FFF | pattern table 0  卡带     |
| 0x1000 ~ 0x1FFF | pattern table 1  卡带     |
| 0x2000 ~ 0x23FF | nametable 0      卡带     |
| 0x2400 ~ 0x27FF | nametable 1      卡带     |
| 0x2800 ~ 0x2BFF | nametable 2      卡带     |
| 0x2C00 ~ 0x2FFF | nametable 3      卡带     |
| 0x3F00 ~ 0x3F1F | palette ram 索引，ppu内部 |
| 0x3F20 ~ 0x3FFF | palette ram 镜像          |

参考：https://www.nesdev.org/wiki/PPU_memory_map
