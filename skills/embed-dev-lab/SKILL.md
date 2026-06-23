---
name: embed-dev-lab
description: STM32F103 embed-dev-lab 开发规范 — probe-rs 烧录、Backup 域 PC13、脚本与 MCP 安全约束。在 embed-dev-lab 仓库内开发 F103 裸机/嵌入式任务时使用。
---

# embed-dev-lab 嵌入式开发 Skill

## 适用场景

- 仓库：`embed-dev-lab`（STM32F103C8T6 核心板、probe-rs、CMake 模块）
- Agent：Cursor / Claude Code / CodeWhale（Codex 无 MCP，读 `AGENTS.md` 摘要）

## 构建与烧录（CLI 保持英文输出）

```bash
./scripts/build.sh f103-blink build
./scripts/build.sh f103-blink flash    # 不自动 compile，改代码后先 build
./scripts/build-flash.sh               # 一键 build + flash；编译失败暂停
```

| 项 | 值 |
|----|-----|
| probe-rs chip | `STM32F103C8Tx` |
| download 格式 | `--binary-format elf`（勿用废弃的 `--format`） |
| 产物 | `modules/f103-blink/build/f103-blink.elf` |

一键环境：`./scripts/bootstrap.sh`（默认不含 MCP；加 `--with-mcp` 安装 embedded-debugger）。

## 硬件要点（F103 PC13）

PC13 属于 **Backup 域**，GPIO 配置前必须：

1. `RCC_APB1ENR.PWREN` — 开启 PWR 时钟  
2. `PWR_CR.DBP` — 解除 Backup 域写保护  

否则 `GPIOC_CRH` 写入无效，LED 不亮。详见 [`doc/reference/stm32f103/md/topics/backup-domain-pc13.md`](doc/reference/stm32f103/md/topics/backup-domain-pc13.md)。

多数核心板 **低电平点亮** PC13 LED。

## Windows ST-Link

- WinUSB：`./scripts/install/stlink-winusb-windows.sh --check-only` / `--install`
- 驱动包：`vendor-pack/STLink/STLink/USBDriver/`
- `probe-rs list` 应见 `STLink V2 -- 0483:3748`

## embedded-debugger MCP 使用约束

- 底层：**probe-rs**（与 `scripts/build.sh flash` 一致）
- **禁止**未经用户确认的全片 Flash 擦除或 mass erase
- 寄存器读写须注明来源（RM0008 topic MD 或 PDF 页码），勿臆造位域
- 烧录前确认已 `build` 最新 ELF
- RTT/调试前确认目标板已供电且 SWD 连接正常

## 文档索引

| 文档 | 路径 |
|------|------|
| 快速上手 | `doc/getting-started.md` |
| probe-rs | `doc/probe-rs.md` |
| CMSIS 与手写边界 | `doc/learn/cmsis-overview.md` |
| ST F1 软件仓库归纳 | `doc/learn/stm32-cmsis-component-repos.md` |
| STM32CubeF1 参考 | https://github.com/STMicroelectronics/STM32CubeF1 |
| HAL 参考 | https://github.com/STMicroelectronics/stm32f1xx-hal-driver · `vendor-pack/stm32f1xx-hal-driver.embed-dev-lab.md` |
| 脚本 | `doc/scripts-reference.md` |
| MCP/Skill | `doc/mcp-skills.md` |
| ST 官方参考 | `doc/reference/stm32f103/` |
| 项目记忆 | `PROJECT_MEMORY.md` |

## 注释与语言

- **源码注释**：中文  
- **终端 / CLI / 日志**：英文  

## 代理

脚本默认 HTTP 代理 `http://127.0.0.1:7890`；离线用 `--no-proxy`。
