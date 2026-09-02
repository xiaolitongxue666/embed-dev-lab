---
name: embed-dev-lab
description: STM32F103 embed-dev-lab 开发规范 — probe-rs 烧录、Backup 域 PC13、CH341 串口 Windows↔WSL、Win serial-ch341-read、WSL picocom、脚本与 MCP 安全约束。在 embed-dev-lab 仓库内开发 F103 裸机/嵌入式任务、验证串口日志时使用。
---

# embed-dev-lab 嵌入式开发 Skill

## 适用场景

- 仓库：`embed-dev-lab`（STM32F103C8T6 核心板、probe-rs、CMake 模块）
- Agent：Cursor / Claude Code / CodeWhale（Codex 无 MCP，读 `AGENTS.md` 摘要）

## 构建与烧录（CLI 保持英文输出）

```bash
./scripts/build.sh f103-manual-reg build
./scripts/build.sh f103-manual-reg flash    # 不自动 compile，改代码后先 build
./scripts/build.sh f103-cmsis-hal build     # 须先 fetch-f103-cmsis-hal-deps.sh
./scripts/build.sh f103-cmsis-hal flash
./scripts/build-flash.sh               # 一键 build + flash；默认 f103-manual-reg
./scripts/build-flash.sh f103-cmsis-hal
```

| 工程 | 实现 | 串口 | 产物 |
|------|------|------|------|
| `f103-manual-reg` | 手写寄存器路线（无 CMSIS/HAL） | `printf` + `syscalls.c` | `projects/f103-manual-reg/build/f103-manual-reg.elf` |
| `f103-cmsis-hal` | CMSIS+HAL / Cube 风格对照路线（非 CubeMX 生成） | `HAL_UART_Transmit`（无 printf） | `projects/f103-cmsis-hal/build/f103-cmsis-hal.elf` |

两工程为**同一芯片上的并列路线**，对外具体功能应对齐；详见 `.cursor/rules/embed-dev-lab-core.mdc`。

| 项 | 值 |
|----|-----|
| probe-rs chip | `STM32F103C8Tx` |
| download 格式 | `--binary-format elf`（勿用废弃的 `--format`） |

一键环境：`./scripts/bootstrap.sh`（默认不含 MCP；加 `--with-mcp` 安装 embedded-debugger）。

编写 → 编译 → 下载：[`doc/workflow-write-build-flash.md`](doc/workflow-write-build-flash.md)（含「烧写脚本在哪里」）。真正烧录在 `scripts/build.sh flash`；一键用 `scripts/build-flash.sh`。`flash` 不 configure；`clean` 后须先 `./scripts/build.sh <module>`。

CH341 串口：`serial-ch341-switch.sh`（to-win/to-wsl）。Windows Agent 读：`./scripts/serial-ch341-read.sh [--baud N]`（自动 COM；波特/端口可变）。WSL：`picocom -b <固件波特> /dev/ttyUSB*`。规则：`.cursor/rules/serial-ch341.mdc`。

## 硬件要点（F103 PC13）

PC13 属于 **Backup 域**，GPIO 配置前必须：

1. `RCC_APB1ENR.PWREN` — 开启 PWR 时钟  
2. `PWR_CR.DBP` — 解除 Backup 域写保护  

否则 `GPIOC_CRH` 写入无效，LED 不亮。详见 [`doc/reference/stm32f103/md/topics/backup-domain-pc13.md`](doc/reference/stm32f103/md/topics/backup-domain-pc13.md)。

多数核心板 **低电平点亮** PC13 LED。

## 串口验证（CH341）

烧录后看日志遵循 `.cursor/rules/serial-ch341.mdc`（**波特率与 COM/tty 名会变**）：

1. Windows Agent：`./scripts/serial-ch341-read.sh`（自动 to-win + 按 VID 找 COM；固件改波特加 `--baud`）
2. WSL：`to-wsl` 后 `picocom -b <与固件一致> /dev/ttyUSB*`
3. 用户 SecureCRT 等：`to-win`；Agent 勿驱动 GUI、勿写死 COMx

详解：`doc/scripts-reference.md`。

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
| 编写/编译/下载流程 | `doc/workflow-write-build-flash.md` |
| 快速上手 | `doc/getting-started.md` |
| 应用层模块 | `doc/projects/` · `projects/README.md` |
| probe-rs | `doc/probe-rs.md` |
| CMSIS 与手写边界 | `doc/learn/cmsis-overview.md` |
| GPIO 八种模式 | `doc/learn/gpio-eight-modes.md` |
| GPIO 保护与负压 | `doc/learn/gpio-protection-and-negative-voltage.md` |
| UART / TTL / RS232 / RS485 | `doc/learn/uart-ttl-rs232-rs485.md` |
| SWD ≠ USART | `doc/learn/swd-vs-usart.md` |
| 裸机 printf / nosys / HAL 串口 | `doc/learn/newlib-nosys-stdio-retarget.md` |
| ST F1 软件仓库归纳 | `doc/learn/stm32-cmsis-component-repos.md` |
| STM32CubeF1 参考 | https://github.com/STMicroelectronics/STM32CubeF1 |
| HAL submodule | `vendor-pack/stm32f1xx-hal-driver` · `v1.1.8` · `./scripts/fetch-cmsis.sh` |
| HAL 参考 | https://github.com/STMicroelectronics/stm32f1xx-hal-driver · `vendor-pack/stm32f1xx-hal-driver.embed-dev-lab.md` |
| 脚本 / CH341 读串口 | `doc/scripts-reference.md` · `serial-ch341-read.sh` · 规则 `serial-ch341.mdc` |
| MCP/Skill | `doc/mcp-skills.md` |
| ST 官方参考 | `doc/reference/stm32f103/` |
| 项目记忆 | `PROJECT_MEMORY.md` |

## 注释与语言

- **源码注释**：`f103-manual-reg` 与 `f103-cmsis-hal` 的工程维护文件（`src/`、`startup/`、`linker/`）中文；`third_party/**` 保持 vendor 英文  
- **仅改注释**：翻译/注释任务不得改动代码逻辑  
- **fetch 后恢复**：`fetch-f103-cmsis-hal-deps.sh` 拷贝后调用 `scripts/lib/apply-f103-cmsis-hal-comments.sh` 恢复中文注释  
- **终端 / CLI / 日志**：英文  

## 代理

脚本默认 HTTP 代理 `http://127.0.0.1:7890`；离线用 `--no-proxy`。
