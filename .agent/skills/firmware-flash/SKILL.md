---
name: firmware-flash
description: 烧录 STM32F103 固件时使用。probe-rs download、build.sh flash、build-flash.sh、ST-Link。禁止 both flash，禁止未经确认的全片擦除。
---

# 烧写

先做 [board-power](../board-power/SKILL.md)：蓝板 MicroUSB 已供电，ST-Link 只接 SWD。改过代码则先按 [firmware-build](../firmware-build/SKILL.md) 编译成功。`flash` 不会编译。

芯片：`STM32F103C8Tx`。格式：`--binary-format elf`。不要用 `--format`。

`<module>` 可以是 `f103-manual-reg`、`f103-cmsis-hal` 或 `f103-manual-rtos`。一块板同一时间只驻留一份固件。

## 只烧录（源码未改，ELF 已是最新）

```bash
./scripts/build.sh f103-manual-reg flash
./scripts/build.sh f103-cmsis-hal flash
./scripts/build.sh f103-manual-rtos flash
```

`flash` 执行 probe-rs download，然后 probe-rs reset。ELF 路径：

- `projects/f103-manual-reg/build/f103-manual-reg.elf`
- `projects/f103-cmsis-hal/build/f103-cmsis-hal.elf`
- `projects/f103-manual-rtos/build/f103-manual-rtos.elf`

## 改过代码：先编译再烧

```bash
./scripts/build.sh <module> build && ./scripts/build.sh <module> flash
```

或一键包装（先 `build` 再 `flash`，不 configure；默认模块 `f103-manual-reg`）：

```bash
./scripts/build-flash.sh
./scripts/build-flash.sh f103-manual-reg
./scripts/build-flash.sh f103-cmsis-hal
./scripts/build-flash.sh f103-manual-rtos
```

`build/` 不存在时 `build-flash.sh` 会失败并提示先跑 `./scripts/build.sh <module>`。编译失败时 ninja/gcc 报错原样输出；交互终端会等待 Enter。不要在编译失败后继续烧。

## 不要做

- `./scripts/build.sh both flash` 与 `./scripts/build-flash.sh both`：脚本拒绝
- 未经用户确认的全片擦除或 mass erase。CLI 与 embedded-debugger MCP 都一样
- `bootstrap.sh` 只装环境并编译，不含烧录

已有备选：`./scripts/build.sh <module> flash-openocd` 烧 hex。默认路径是上面的 probe-rs ELF。

## MCP

embedded-debugger 与 `scripts/build.sh flash` 一样走 probe-rs。使用前：

1. ELF 已是这次改动的产物
2. 板子已供电，SWD 已接好
3. 寄存器读写注明 RM0008 topic 或 PDF 页码
4. 用户没有明确要求时，不调用全片擦除

安装与配置见 [`doc/mcp-skills.md`](../../../doc/mcp-skills.md)。本 Skill 不改 `.cursor/mcp.json`。

## 失败分支

| 现象 | 处理 |
|------|------|
| `probe-rs list` 为空 | 先看设备管理器。若是 `USB\VID_0000&PID_0002\...`、状态为错误、文案为设备描述符请求失败：`pnputil /restart-device "<该实例 ID>"`。不要对 `0000:0002` 装 WinUSB。重启后应为 `STLink V2 -- 0483:3748`。已经是 `0483:3748` 仍打不开，再 `./scripts/install/stlink-winusb-windows.sh --check-only`，需要时 `--install`。驱动包在 `vendor-pack/STLink/STLink/USBDriver/` |
| `Failed to open probe` / USB error 5 | ST-Link 已被另一次 probe-rs 或 gdb 占用。等这次 `flash` 结束，或结束 `probe-rs.exe` 与 `arm-none-eabi-gdb.exe` 后再下载。不要并行。`gdb -batch` 里的 `while` 会留下交互提示并占住探针 |
| 停核之后板子不再跑 | `probe-rs gdb` 的 detach 可能把核留在暂停。看完执行 `probe-rs reset --chip STM32F103C8Tx`。读 USART/SCB 的步骤在 [`doc/probe-rs.md`](../../../doc/probe-rs.md) |
| `JtagGetIdcodeError` | 回到 [board-power](../board-power/SKILL.md)。MCU 未上电或 SWD 未接 |
| 烧完行为仍是旧的 | 源码改过但没有先 `build`。回到编译 Skill |
| 要看日志 | [serial-console](../serial-console/SKILL.md)。串口与 SWD 是两根线 |

IDE 任务和 F5 也落到这些脚本，见 [`doc/ide-debug.md`](../../../doc/ide-debug.md)、[`doc/probe-rs.md`](../../../doc/probe-rs.md)。
