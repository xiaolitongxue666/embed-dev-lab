# embed-dev-lab 文档

中文使用说明；终端命令与 CLI 输出保持英文。

## 推荐阅读顺序

1. [快速上手](getting-started.md) — bootstrap → 接线 → 烧录 → 看 LED / 串口  
2. [编写 → 编译 → 下载](workflow-write-build-flash.md) — 日常改代码、**烧写脚本在哪里**、两工程命令  
3. [probe-rs 指南](probe-rs.md) — CLI、WinUSB、ST-Link、排错  
4. [IDE 调试与扩展](ide-debug.md) — probe-rs-debugger、launch/tasks  
5. [脚本参考](scripts-reference.md) — 全脚本、烧写入口、CH341 宿主切换、WSL picocom、自动化链路  
6. [F103 硬件外设与接线](hardware/stm32f103-peripherals.md) — OLED / ADC / LM75A 计划接线、引脚核对、采购  
7. [f103-manual-reg 模块](projects/f103-manual-reg.md) — 源码与硬件要点  
8. [f103-cmsis-hal 模块](projects/f103-cmsis-hal.md) — CubeIDE 风格对照（CMSIS+HAL）  
9. [f103-manual-reg 从零手写构建](learn/f103-manual-build-from-scratch.md) — 文件编写顺序、CMSIS 对照、验收清单  
10. [f103-manual-reg 编译流程](learn/f103-module-build-flow.md) — CMake、.c/.s 链接、startup 与链接脚本  
11. [链接器 VMA/LMA](learn/linker-vma-lma.md) — `.data` 分离、AT>FLASH、与 startup 协作  
12. [裸机 printf / nosys / HAL 串口](learn/newlib-nosys-stdio-retarget.md) — `_write`、两工程串口选型  
13. [STM32F103 内存映射与启动流程](learn/stm32f103-memory-boot-map.md) — BOOT 重映射、Flash/SRAM、复位加载  
14. [STM32F103 MMIO 基础](learn/stm32f103-mmio-basics.md) — 外设寄存器、PC13 点灯  
15. [链接器 Map 文件](learn/linker-map-file.md) — 内存布局报告、段与符号  
16. [STM32F103 官方参考](reference/stm32f103/README.md) — DS5319 + RM0008  
17. [STM32 裸机入门笔记](learn/stm32-bare-metal-bootstrap.md) — startup、SystemInit、RCC、HSE  
18. [CMSIS 标准与手写裸机边界](learn/cmsis-overview.md) — 分层、手写兼容判定、与 HAL 关系  
19. [ST F1 软件仓库归纳](learn/stm32-cmsis-component-repos.md) — CMSIS / HAL / STM32CubeF1  
20. [中断向量表与 NVIC](learn/interrupt-vector-table-and-nvic.md) — 向量表、NVIC、与 startup 关系  
21. [MCP 与 Skill](mcp-skills.md) — embedded-debugger MCP、项目 Skill 安装

## 应用层

| 文档 | 内容 |
|------|------|
| [projects/README.md](projects/README.md) | 固件小工程索引（与 `projects/` 目录对应） |
| [projects/f103-manual-reg.md](projects/f103-manual-reg.md) | F103 PC13 全手写寄存器 demo |
| [projects/f103-cmsis-hal.md](projects/f103-cmsis-hal.md) | CMSIS+HAL CubeIDE 风格对照 demo |

## 硬件

| 文档 | 内容 |
|------|------|
| [hardware/stm32f103-peripherals.md](hardware/stm32f103-peripherals.md) | F103 外设清单、接线、引脚冲突核对、采购链接 |

## 文档列表

| 文档 | 内容 |
|------|------|
| [getting-started.md](getting-started.md) | 新手线性流程 |
| [workflow-write-build-flash.md](workflow-write-build-flash.md) | 编写 → 编译 → 下载 |
| [hardware/stm32f103-peripherals.md](hardware/stm32f103-peripherals.md) | F103 硬件外设与接线 |
| [probe-rs.md](probe-rs.md) | probe-rs CLI 与驱动 |
| [ide-debug.md](ide-debug.md) | Cursor/VS Code 插件与调试 |
| [scripts-reference.md](scripts-reference.md) | scripts/ 脚本说明 |
| [projects/f103-manual-reg.md](projects/f103-manual-reg.md) | F103 PC13 全手写寄存器 demo |
| [projects/f103-cmsis-hal.md](projects/f103-cmsis-hal.md) | CMSIS+HAL CubeIDE 风格对照 |
| [learn/f103-manual-build-from-scratch.md](learn/f103-manual-build-from-scratch.md) | 从零手写：文件顺序、CMSIS 对照、验收清单 |
| [learn/f103-module-build-flow.md](learn/f103-module-build-flow.md) | CMake 构建、.c/.s 链接、startup 与链接脚本协作 |
| [learn/linker-vma-lma.md](learn/linker-vma-lma.md) | VMA/LMA、`.data` AT>FLASH |
| [learn/newlib-nosys-stdio-retarget.md](learn/newlib-nosys-stdio-retarget.md) | printf / nosys / HAL 串口选型 |
| [learn/stm32f103-memory-boot-map.md](learn/stm32f103-memory-boot-map.md) | BOOT 重映射、Flash/SRAM/System memory、复位加载 |
| [learn/stm32f103-mmio-basics.md](learn/stm32f103-mmio-basics.md) | MMIO、PC13 点灯、PPB vs ST 外设 |
| [learn/linker-map-file.md](learn/linker-map-file.md) | 链接器 map：段/符号、排查与优化 |
| [reference/stm32f103/](reference/stm32f103/README.md) | ST 官方 Datasheet / RM + 精选主题 |
| [learn/stm32-bare-metal-bootstrap.md](learn/stm32-bare-metal-bootstrap.md) | startup、SystemInit、RCC、HSE |
| [learn/cmsis-overview.md](learn/cmsis-overview.md) | CMSIS 分层、手写兼容边界、与 HAL 关系 |
| [learn/stm32-cmsis-component-repos.md](learn/stm32-cmsis-component-repos.md) | ST F1：cmsis-core / cmsis-device-f1 / HAL / CubeF1 |
| [learn/interrupt-vector-table-and-nvic.md](learn/interrupt-vector-table-and-nvic.md) | 中断向量表、NVIC |
| [learn/datasheet-vs-reference-manual.md](learn/datasheet-vs-reference-manual.md) | DS5319 与 RM0008 分工 |
| [mcp-skills.md](mcp-skills.md) | embedded-debugger MCP + 项目 Skill |

## 其他

- 项目入口：[README.md](../README.md)  
- 维护速查：[PROJECT_MEMORY.md](../PROJECT_MEMORY.md)（Agent / 维护者）
