# embed-dev-lab 文档

中文使用说明；终端命令与 CLI 输出保持英文。

## 推荐阅读顺序

1. [快速上手](getting-started.md) — bootstrap → 接线 → 烧录 → 看 LED  
2. [probe-rs 指南](probe-rs.md) — CLI、WinUSB、ST-Link、排错  
3. [IDE 调试与扩展](ide-debug.md) — probe-rs-debugger、launch/tasks  
4. [脚本参考](scripts-reference.md) — 全脚本与自动化链路  
5. [f103-manual-reg 模块](projects/f103-manual-reg.md) — 源码与硬件要点  
6. [f103-manual-reg 编译流程](learn/f103-module-build-flow.md) — CMake、.c/.s 链接、startup 与链接脚本  
7. [链接器 Map 文件](learn/linker-map-file.md) — 内存布局报告、段与符号、排查与优化  
8. [STM32F103 官方参考](reference/stm32f103/README.md) — DS5319 + RM0008（fetch 脚本 + 精选 MD）  
9. [STM32 裸机入门笔记](learn/stm32-bare-metal-bootstrap.md) — startup、SystemInit 调用链、ARM 汇编与 x86 对比、RCC、HSE/HSERDY、CMSIS、资料分工  
10. [CMSIS 标准与手写裸机边界](learn/cmsis-overview.md) — 分层、CubeMX、手写兼容判定、与 HAL 关系  
11. [ST F1 软件仓库归纳](learn/stm32-cmsis-component-repos.md) — CMSIS / HAL / STM32CubeF1 参考 repo  
12. [中断向量表与 NVIC](learn/interrupt-vector-table-and-nvic.md) — 向量表作用、NVIC 职责、与 CMSIS/HAL 关系  
13. [MCP 与 Skill](mcp-skills.md) — embedded-debugger MCP、项目 Skill 安装

## 应用层

| 文档 | 内容 |
|------|------|
| [projects/README.md](projects/README.md) | 固件小工程索引（与 `projects/` 目录对应） |
| [projects/f103-manual-reg.md](projects/f103-manual-reg.md) | F103 PC13 全手写寄存器 demo |
| [projects/f103-cmsis-hal.md](projects/f103-cmsis-hal.md) | CMSIS+HAL 占位 |

## 文档列表

| 文档 | 内容 |
|------|------|
| [getting-started.md](getting-started.md) | 新手线性流程 |
| [probe-rs.md](probe-rs.md) | probe-rs CLI 与驱动 |
| [ide-debug.md](ide-debug.md) | Cursor/VS Code 插件与调试 |
| [scripts-reference.md](scripts-reference.md) | scripts/ 脚本说明 |
| [projects/f103-manual-reg.md](projects/f103-manual-reg.md) | F103 PC13 全手写寄存器 demo |
| [projects/f103-cmsis-hal.md](projects/f103-cmsis-hal.md) | CMSIS+HAL 占位 |
| [learn/f103-module-build-flow.md](learn/f103-module-build-flow.md) | CMake 构建、.c/.s 链接、startup 与链接脚本协作 |
| [learn/linker-map-file.md](learn/linker-map-file.md) | 链接器 map：内存清单、段/符号、排查与优化、生成方式 |
| [reference/stm32f103/](reference/stm32f103/README.md) | ST 官方 Datasheet / RM + 精选主题 |
| [learn/stm32-bare-metal-bootstrap.md](learn/stm32-bare-metal-bootstrap.md) | startup、SystemInit 调用链、ARM 汇编与 x86 对比、RCC、HSE/HSERDY 轮询、CMSIS 简述、资料分工 |
| [learn/cmsis-overview.md](learn/cmsis-overview.md) | CMSIS 6 / 分层、CubeMX、手写兼容边界、与 HAL 关系 |
| [learn/stm32-cmsis-component-repos.md](learn/stm32-cmsis-component-repos.md) | ST F1：cmsis-core / cmsis-device-f1 / stm32f1xx-hal-driver / STM32CubeF1 |
| [learn/interrupt-vector-table-and-nvic.md](learn/interrupt-vector-table-and-nvic.md) | 中断向量表、NVIC、与 startup / CMSIS / HAL 关系 |
| [learn/datasheet-vs-reference-manual.md](learn/datasheet-vs-reference-manual.md) | DS5319 与 RM0008 分工、改 system 该看哪本 |
| [mcp-skills.md](mcp-skills.md) | embedded-debugger MCP + 项目 Skill |

## 其他

- 项目入口：[README.md](../README.md)  
- 维护速查：[PROJECT_MEMORY.md](../PROJECT_MEMORY.md)（Agent / 维护者）
