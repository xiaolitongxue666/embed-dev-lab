# UART、TTL 与 RS232 / RS485 / RS422

整理自学习笔记，区分 **片内 UART/USART 外设**、**TTL/CMOS 引脚电平**，以及 **RS232 / RS485 / RS422 总线电气标准**。对照本仓库 USART1 + CH341 USB-TTL。SWD 调试通道与串口无关，见 [swd-vs-usart.md](swd-vs-usart.md)；printf / HAL 发送路径见 [newlib-nosys-stdio-retarget.md](newlib-nosys-stdio-retarget.md)。

---

## 一句话总结

**UART（USART）是芯片内部外设**，引脚输出的是 **近距离 TTL/CMOS 电平**；**RS232、RS485、RS422 是总线电气标准**（电压、单端/差分、距离、抗干扰）。MCU 不能直接当 RS232/485 用，须经电平转换芯片。本仓库 CH341 是 **USB ↔ TTL**，不是 RS232。

---

## 1. 核心概念区分

| 概念 | 是什么 | 典型电平 / 形态 |
|------|--------|-----------------|
| **UART / USART** | 芯片内部异步串口外设（帧格式、波特率、移位） | 逻辑层；经 GPIO 复用出引脚 |
| **TTL / CMOS 电平** | MCU 引脚上的数字高低电平 | 3.3 V 或 5 V 单端；近距离点对点 |
| **RS232** | 单端电气标准 | 典型 ±3～±15 V；需 MAX3232 一类 |
| **RS485** | 差分总线电气标准 | A/B 差分；多机、长线；需 MAX485 一类 |
| **RS422** | 差分点对点（常与 485 同族讨论） | 差分；多为全双工点对点 |

### TTL（Transistor-Transistor Logic）

历史名称：晶体管-晶体管逻辑电平。单片机 UART 引脚直接输出的电平，习惯称为 **3.3 V TTL** 或 **5 V TTL**。

STM32F103 核心板 USART1 引脚实际是 **3.3 V CMOS**，与传统 5 V TTL 不完全相同，但业界常统称「TTL 串口」，表示「MCU 侧逻辑电平、非 RS232」。

---

## 2. 本仓库对照

| 项 | 值 |
|----|-----|
| 外设 | USART1（片内） |
| 引脚 | PA9 = TX，PA10 = RX（默认映射，复用功能） |
| 引脚电平 | 3.3 V CMOS（习惯称 TTL 兼容） |
| USB 适配 | **CH341 USB-TTL**（USB ↔ TTL，**不是** RS232） |
| 接线 | 模块 **RX ← PA9**，TX → PA10，**GND 共地** |
| 波特 | 以固件为准（demo 常见 1500000 8N1） |

规则与脚本： [`.cursor/rules/serial-ch341.mdc`](../../.cursor/rules/serial-ch341.mdc) · [`doc/scripts-reference.md`](../scripts-reference.md)。

```text
PC ──USB──► CH341 ──TTL TX/RX/GND──► STM32 USART1（PA10/PA9）
              ↑
         电平仍是 3.3 V 逻辑，不是 RS232 ±12 V
```

---

## 3. 为何不能直连 RS232 / RS485

| 标准 | 若直连 MCU | 正确做法 |
|------|------------|----------|
| **RS232** | ± 十几伏会损坏 3.3 V IO | MAX3232 等：TTL ↔ RS232 |
| **RS485** | 差分、驱动能力、多机协议均不同 | MAX485 / SP3485 等：TTL ↔ A/B |
| **RS422** | 同上（差分） | 对应收发器芯片 |

UART 负责「比特怎么排、波特多少」；转换芯片负责「线上电压与拓扑」。

---

## 4. 与 SWD、ST-Link 虚拟串口

- **SWD**（PA13/PA14）只用于下载与调试，**不能**当 USART 收发。见 [swd-vs-usart.md](swd-vs-usart.md)。
- 部分 ST-Link 带 **VCP（虚拟 COM）**，须把 MCU USART 脚接到 ST-Link 的 VCP TX/RX；本仓库核心板日志走 **独立 CH341**，与 SWD 独立。

---

## 5. 误区速查

| 误区 | 纠正 |
|------|------|
| ❌ UART 就是 RS232 | ✅ UART 是协议/外设；RS232 是电气标准 |
| ❌ CH341 输出 RS232 | ✅ 常见 CH341 模块是 USB-TTL |
| ❌ 接了 USB-TTL 就等于有 RS485 | ✅ 485 要差分收发器与 A/B 总线 |
| ❌ 串口必须经 SWD | ✅ 串口硬件运行不依赖 SWD |

---

## 延伸阅读

| 主题 | 文档 |
|------|------|
| SWD ≠ 串口 | [swd-vs-usart.md](swd-vs-usart.md) |
| printf / HAL 发送 | [newlib-nosys-stdio-retarget.md](newlib-nosys-stdio-retarget.md) |
| 接线与 CH341 | [stm32f103-peripherals.md](../hardware/stm32f103-peripherals.md) · [getting-started.md](../getting-started.md) |
| C8T6 引脚总表 | [stm32f103c8t6-pinout.md](../hardware/stm32f103c8t6-pinout.md) |
| GPIO 复用推挽 / 浮空 RX | [gpio-eight-modes.md](gpio-eight-modes.md) |
| 源码 | [`usart.c`](../../projects/f103-manual-reg/src/usart.c) |
