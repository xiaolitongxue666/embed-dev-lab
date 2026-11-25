---
name: embed-dev-lab
description: STM32F103 embed-dev-lab 开发规范 — 选择编译、烧写、串口或上电 Skill。probe-rs、CH341、双路线、禁止未确认全片擦除。在本仓库做 F103 裸机任务时使用。
---

# embed-dev-lab Skill

本文件是任务入口。先选下面的操作 Skill，再按其中的命令做。不要只读本页就去编译或烧录。

| 要做的事 | Skill |
|----------|--------|
| 编译、`clean` 之后、cmsis-hal 缺头文件 | [firmware-build](../firmware-build/SKILL.md) |
| 烧录、probe-rs、ST-Link、MCP 下载 | [firmware-flash](../firmware-flash/SKILL.md) |
| CH341、COM、`ttyUSB`、看日志 | [serial-console](../serial-console/SKILL.md) |
| 烧录或看日志前确认供电和共地 | [board-power](../board-power/SKILL.md) |

顺序与失败回路：[`.agent/workflows/write-build-flash.md`](../../workflows/write-build-flash.md)。

## 硬约束

- 双路线：`f103-manual-reg`（手写寄存器）与 `f103-cmsis-hal`（CMSIS+HAL，Cube 风格，非 CubeMX 生成）。对外行为对齐，除非用户明确只改一边。规则：[`.agent/rules/architecture.md`](../../rules/architecture.md)
- `f103-manual-rtos`：手写 SysTick + PendSV，外设同 manual-reg。不参与双路线同步，`both` 不编译它。任务栈保持 384 字。切到 PSP 用 `MSR PSP` 和异常返回值 `0xFFFFFFFD`，不写 CONTROL，PendSV 里不用 BASEPRI。规则：[`.agent/rules/f103-manual-rtos.md`](../../rules/f103-manual-rtos.md)。演示：[时间片与双栈](../../../doc/learn/f103-manual-rtos-timeslice.html)、[PendSV 逐指令](../../../doc/learn/f103-manual-rtos-pendsv.html)
- `bytes=0` 时先按 serial-console 对照 USART 寄存器，不要改接线。`probe-rs list` 为空时先按 firmware-flash 区分描述符失败和 WinUSB
- Windows 脚本在 Git Bash 运行
- `./scripts/build.sh <module> flash` 不编译。改代码后先 build
- 禁止未经用户确认的全片 Flash 擦除或 mass erase
- 串口 COM / tty 名和波特率都会变。不要写死端口，也不要假定永远是 1500000
- PA13/PA14 只做 SWD
- 注释：`src/`、`startup/`、`linker/` 中文；`third_party/**` 英文；CLI 英文

## 路线差异（做功能时）

| 工程 | 串口 | 时钟 | 产物 |
|------|------|------|------|
| `f103-manual-reg` | `LOG_*` → `printf` + `syscalls.c` → DMA TX；RX 为 DMA + IDLE | `SystemInit` 里升到 72 MHz | `projects/f103-manual-reg/build/f103-manual-reg.elf` |
| `f103-cmsis-hal` | `LOG_*` → 一次 `HAL_UART_Transmit`（不要加 printf/syscalls） | `main` 里 `SystemClock_Config` | `projects/f103-cmsis-hal/build/f103-cmsis-hal.elf` |
| `f103-manual-rtos` | 同 manual-reg，多任务发送加 `USART1_Lock` | `SystemInit` 72 MHz；SysTick 由 `os.c` 独占 | `projects/f103-manual-rtos/build/f103-manual-rtos.elf` |

PC13 属于 Backup 域。manual-reg 先 `RCC_APB1ENR.PWREN` + `PWR_CR.DBP`。cmsis-hal 先 `HAL_PWR_EnableBkUpAccess()`。多数核心板 PC13 低电平点亮。

## 代理

脚本默认 HTTP 代理 `http://127.0.0.1:7890`。离线给带代理的脚本加 `--no-proxy`。

## 还要读

- 规则：[`.agent/rules/`](../../rules/)
- 记忆索引：[`PROJECT_MEMORY.md`](../../../PROJECT_MEMORY.md)
- 人类流程：[`doc/workflow-write-build-flash.md`](../../../doc/workflow-write-build-flash.md)
