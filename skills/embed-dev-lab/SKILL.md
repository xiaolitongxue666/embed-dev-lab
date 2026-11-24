---
name: embed-dev-lab
description: STM32F103 embed-dev-lab 开发规范 — probe-rs 烧录、Backup 域 PC13、CH341 串口 Windows↔WSL、Win serial-ch341-read、WSL picocom、脚本与 MCP 安全约束。在 embed-dev-lab 仓库内开发 F103 裸机/嵌入式任务、验证串口日志时使用。
---

# embed-dev-lab Skill

操作步骤的正文在 [`.agent/skills/embed-dev-lab/SKILL.md`](../../.agent/skills/embed-dev-lab/SKILL.md)。本文件只保留旧路径。

| 要做的事 | Skill |
|----------|--------|
| 编译 | [`.agent/skills/firmware-build/SKILL.md`](../../.agent/skills/firmware-build/SKILL.md) |
| 烧写 | [`.agent/skills/firmware-flash/SKILL.md`](../../.agent/skills/firmware-flash/SKILL.md) |
| 串口 | [`.agent/skills/serial-console/SKILL.md`](../../.agent/skills/serial-console/SKILL.md) |
| 上电与共地 | [`.agent/skills/board-power/SKILL.md`](../../.agent/skills/board-power/SKILL.md) |

## 硬约束

- Windows 脚本在 Git Bash 运行
- `./scripts/build.sh <module> flash` 不编译。改代码后先 build
- 禁止未经用户确认的全片 Flash 擦除或 mass erase
- 串口不要写死 COM/tty 名，波特率以当前固件为准
