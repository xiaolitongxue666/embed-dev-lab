# 工具

Tool 是脚本和 MCP。Skill 规定何时调用。本文件只列入口，不增加新参数。

| 能力 | 入口 |
|------|------|
| 编译 | `./scripts/build.sh <module> [all\|configure\|build\|clean]` |
| 烧录 | `./scripts/build.sh <module> flash` |
| 编译并烧录 | `./scripts/build-flash.sh [module]` |
| CMSIS/HAL 依赖 | `./scripts/fetch-cmsis.sh`，然后 `./scripts/fetch-f103-cmsis-hal-deps.sh` |
| CH341 宿主 | `./scripts/serial-ch341-switch.sh status\|to-win\|to-wsl` |
| Windows 读串口 | `./scripts/serial-ch341-read.sh` |
| WSL 读串口 | `picocom`（先 `to-wsl`） |
| 环境 | `./scripts/bootstrap.sh`（不含烧录） |
| ST-Link WinUSB | `./scripts/install/stlink-winusb-windows.sh` |
| 片上调试 | embedded-debugger MCP，配置 [`.cursor/mcp.json`](../../.cursor/mcp.json)，安装 `./scripts/install-mcp-skills.sh` |

调用步骤见 [`.agent/skills/`](../skills/)。脚本行为说明见 [`doc/scripts-reference.md`](../../doc/scripts-reference.md)。
