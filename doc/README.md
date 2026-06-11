# embed-dev-lab 文档

中文使用说明；终端命令与 CLI 输出保持英文。

## 推荐阅读顺序

1. [快速上手](getting-started.md) — bootstrap → 接线 → 烧录 → 看 LED  
2. [probe-rs 指南](probe-rs.md) — CLI、WinUSB、ST-Link、排错  
3. [IDE 调试与扩展](ide-debug.md) — probe-rs-debugger、launch/tasks  
4. [脚本参考](scripts-reference.md) — 全脚本与自动化链路  
5. [f103-blink 模块](modules-f103-blink.md) — 源码与硬件要点  
6. [STM32F103 官方参考](reference/stm32f103/README.md) — DS5319 + RM0008（fetch 脚本 + 精选 MD）  
7. [STM32 裸机入门笔记](learn/stm32-bare-metal-bootstrap.md) — startup、RCC、CMSIS、资料分工  
8. [MCP 与 Skill](mcp-skills.md) — embedded-debugger MCP、项目 Skill 安装

## 文档列表

| 文档 | 内容 |
|------|------|
| [getting-started.md](getting-started.md) | 新手线性流程 |
| [probe-rs.md](probe-rs.md) | probe-rs CLI 与驱动 |
| [ide-debug.md](ide-debug.md) | Cursor/VS Code 插件与调试 |
| [scripts-reference.md](scripts-reference.md) | scripts/ 脚本说明 |
| [modules-f103-blink.md](modules-f103-blink.md) | F103 PC13 demo |
| [reference/stm32f103/](reference/stm32f103/README.md) | ST 官方 Datasheet / RM + 精选主题 |
| [learn/stm32-bare-metal-bootstrap.md](learn/stm32-bare-metal-bootstrap.md) | startup、RCC、CMSIS、资料分工 |
| [learn/datasheet-vs-reference-manual.md](learn/datasheet-vs-reference-manual.md) | DS5319 与 RM0008 分工、改 system 该看哪本 |
| [mcp-skills.md](mcp-skills.md) | embedded-debugger MCP + 项目 Skill |

## 其他

- 项目入口：[README.md](../README.md)  
- 维护速查：[PROJECT_MEMORY.md](../PROJECT_MEMORY.md)（Agent / 维护者）
