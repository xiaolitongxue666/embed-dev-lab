# 架构

共用正文。Cursor 注入同一约束：[`.cursor/rules/embed-dev-lab-core.mdc`](../../.cursor/rules/embed-dev-lab-core.mdc)。

## 双路线（功能一致，实现路径不同）

同一目标芯片（STM32F103）下并列两套工程，对外具体功能应对齐（LED、串口日志等行为一致），仅实现路径不同：

| 工程 | 路线 |
|------|------|
| `f103-manual-reg` | 手写寄存器：直接操作外设寄存器完成功能；不链接官方 CMSIS Device / HAL。分层 `src/app` `src/board` `src/periph` `src/driver` |
| `f103-cmsis-hal` | CMSIS + HAL：用 ST 官方 CMSIS/HAL，按 STM32Cube / CubeIDE 生成工程的分层与风格手写对照（`MX_*` / MSP / `hal_conf` 等）。非 CubeMX 一键生成、非占位 stub。HAL Src 16 个 `.c` |

新增或变更 demo 行为时，两条路线应同步对齐，除非用户明确只改其一。实现约束见 [`f103-manual-reg.md`](f103-manual-reg.md)、[`f103-cmsis-hal.md`](f103-cmsis-hal.md)。

`f103-manual-rtos` 是手写 SysTick + PendSV 的学习工程，外设行为对照 manual-reg，不参与上面的同步。`./scripts/build.sh both` 不编译它。约束见 [`f103-manual-rtos.md`](f103-manual-rtos.md)。

## 构建与烧录

- Windows 脚本须在 Git Bash 运行
- probe-rs chip：`STM32F103C8Tx`；烧录：`--binary-format elf`（勿用 `--format`）
- `flash` 不自动 compile / configure；改代码先 `build`；`clean` 后先 `./scripts/build.sh <module>`
- `./scripts/build.sh both build` 可回归 manual-reg 与 cmsis-hal；不含 `f103-manual-rtos`。`both flash` 拒绝
- cmsis-hal 依赖：`./scripts/fetch-cmsis.sh` 然后 `./scripts/fetch-f103-cmsis-hal-deps.sh`
- 操作步骤： [`.agent/skills/firmware-build/SKILL.md`](../skills/firmware-build/SKILL.md)、[`.agent/skills/firmware-flash/SKILL.md`](../skills/firmware-flash/SKILL.md)
- 人类流程说明：[`doc/workflow-write-build-flash.md`](../../doc/workflow-write-build-flash.md)

## 相关约束

- 注释与 CLI 语言：[`coding.md`](coding.md)
- 全片擦除、SWD 引脚、串口独占：[`security.md`](security.md)
- 供电与共地：[`hardware.md`](hardware.md)
- CH341：[`serial.md`](serial.md)
- 验收：[`testing.md`](testing.md)
