# SWD（Serial Wire）与 USART 串口

整理自学习笔记：CubeMX 里 **Debug = Serial Wire** 不是「打开串口」；SWD 与 USART 相互独立。对照本仓库 **probe-rs / ST-Link 走 PA13/PA14**，日志走 **USART1 + CH341**。电平标准见 [uart-ttl-rs232-rs485.md](uart-ttl-rs232-rs485.md)；引脚约束见 [stm32f103-peripherals.md](../hardware/stm32f103-peripherals.md)；丝印总表见 [stm32f103c8t6-pinout.md](../hardware/stm32f103c8t6-pinout.md)。

---

## 一句话总结

**SWD 调试通道 ≠ 串口。** 提前保留 Serial Wire（PA13/PA14）的最大价值：一是支持在线调试排错，二是锁定调试脚，防止后期开启调试后串口引脚被抢占而失效。本仓库 **不是 CubeMX 一键生成**，等价约束写在硬件文档：PA13/PA14 **禁止改普通 GPIO**。

---

## 1. 核心概念区分

### Serial Wire（SWD）

在 CubeMX 的 SYS → Debug 选项中：

- **Serial Wire**：2 根线（**PA13 = SWDIO**、**PA14 = SWCLK**），用于下载程序、断点调试
- 与 ST-Link **虚拟串口（VCP）** 相互独立；SWD 通道本身 **不能** 收发 USART 数据
- SWD 只传调试信号；MCU 供电来自蓝板 **MicroUSB**；ST-Link 的 3.3V **不接蓝板**，只给面包板外设（方案 A：5V 闲置）。GND 经面包板轨星型汇集（见 [供电与共地](../hardware/power-and-common-ground.md)）
- 可选：Serial Wire（启用 SWD，推荐）、JTAG、No Debug（关闭调试接口）

> 开启 SWD ≠ 开启串口；串口 USART 需要单独配置外设。

本仓库：

| 通道 | 引脚 | 工具 |
|------|------|------|
| SWD | PA13 / PA14 | ST-Link + probe-rs（烧录 / 调试） |
| USART1 日志 | PA9 / PA10 | CH341 USB-TTL |

### TTL / RS232 / RS485（回顾）

- UART 是单片机异步串口外设，原生输出 TTL/CMOS 电平（单端，TX/RX/GND，近距离点对点）
- RS232/RS422/RS485 属于电平转换标准；详见 [uart-ttl-rs232-rs485.md](uart-ttl-rs232-rs485.md)
- ST-Link 虚拟串口须把 MCU USART 接到 ST-Link VCP 脚；**与 SWD 无关**。本板日志用独立 CH341

---

## 2. 为什么做串口实验要优先保留 Debug = Serial Wire

### 2.1 基础功能

打开 SWD 才能用 ST-Link / probe-rs 下载代码、在线断点调试。若选择 No Debug，无法在线调试，串口出故障只能靠打印信息排查，定位困难。

### 2.2 最经典引脚陷阱（重点）

- PA13、PA14 为 SWD 专用引脚
- 若 Debug 设为 **No Debug**，这两个引脚被释放，CubeMX 允许把 PA13/PA14 配成 GPIO、串口 TX/RX
- 后续一旦改回 Serial Wire，芯片强制收回 PA13、PA14 作调试脚，**原先分配的串口功能直接失效**，代码/硬件都「看起来」没问题，很难排错

✅ **最佳做法（CubeMX）**：最先设置 Debug = Serial Wire，锁定 PA13、PA14，不能再分配给串口，从源头避免引脚冲突。

✅ **本仓库等价做法**：硬件与文档写死 PA13/PA14 仅 SWD，USART1 固定 PA9/PA10；固件不改调试脚。

### 2.3 开发优势

SWD 调试和 USART 串口打印可同时工作：一边断点，一边串口看日志，方便排查波特率、中断、收发异常。

---

## 3. 误区汇总

| 误区 | 纠正 |
|------|------|
| ❌ Debug 选 Serial Wire 就是启用 ST-Link 串口 | ✅ SWD 只是调试下载通道，和串口收发无关；虚拟串口要单独接 USART |
| ❌ 串口功能依赖 SWD | ✅ 串口硬件运行 **不需要** SWD；预先保留 SWD 是开发调试、规避引脚冲突的习惯，不是运行必需 |
| ❌ 关闭 Debug 才能用 PA13/PA14 做串口 | ⚠ 风险：后期开启 SWD 后串口直接失效，不建议 |

---

## 4. 推荐 CubeMX 配置顺序（对照用）

本仓库工程为手写 / Cube 风格对照，**非** CubeMX 一键生成。若用 CubeMX 做类似实验：

1. SYS → Debug = **Serial Wire**（锁定 PA13/14）
2. 配置 RCC 时钟
3. 配置 USART 串口、引脚、NVIC（勿用 PA13/14）
4. 配置时钟树，生成工程

本仓库实际约定见 [硬件 §8 固件摘要](../hardware/stm32f103-peripherals.md#8-固件--cube-配置摘要备查)：`SYS Debug | Serial Wire（保留 SWD）`。

---

## 5. 与本仓库工具链

| 操作 | 依赖 |
|------|------|
| `./scripts/build.sh … flash` / F5 调试 | SWD（ST-Link） |
| `./scripts/serial-ch341-read.sh` / picocom | USART1 + CH341 |
| 仅 LED 闪烁验证 | 可不接串口；SWD 仍建议保留以便烧录 |

串口与 SWD 独立：烧录只需 ST-Link；看日志才需 USB-TTL。见 [workflow-write-build-flash.md](../workflow-write-build-flash.md)。

---

## 延伸阅读

| 主题 | 文档 |
|------|------|
| UART / TTL / RS232 | [uart-ttl-rs232-rs485.md](uart-ttl-rs232-rs485.md) |
| 引脚表与冲突核对 | [stm32f103-peripherals.md](../hardware/stm32f103-peripherals.md) |
| C8T6 丝印引脚总表 | [stm32f103c8t6-pinout.md](../hardware/stm32f103c8t6-pinout.md) |
| probe-rs / SWD 接线 | [probe-rs.md](../probe-rs.md) · [getting-started.md](../getting-started.md) |
| IDE 调试 | [ide-debug.md](../ide-debug.md) |
