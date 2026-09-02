# 供电、上电与 SPI 电气时序

| 字段 | 值 |
|------|-----|
| 来源 | DocID026899 §4.2、§4.4.1；上电 20 ms 见 AN4650 §3 / §5.7 |
| 版本 | 数据手册 Rev 8；AN4650 作 boot 补充 |
| PDF 页码 | DocID026899 p.23、p.25 |
| 整理日期 | 2026-08-31 |
| 源码 | [`projects/f103-manual-reg/src/spi.c`](../../../../projects/f103-manual-reg/src/spi.c)、[`lsm6ds3.c`](../../../../projects/f103-manual-reg/src/lsm6ds3.c) |

## 供电

Table 4（PDF p.23）：

| 符号 | 含义 | Min | Typ | Max | 单位 |
|------|------|-----|-----|-----|------|
| `Vdd` | 模拟供电 | 1.71 | 1.8 | 3.6 | V |
| `Vdd_IO` | IO 供电 | 1.62 | — | Vdd+0.1 | V |

本仓库模块接线：**面包板 3.3V 轨（ST-Link 3.3V）→ 模块 `3V3`**，GND 与蓝板 / ST-Link / CH341 共地；**不要接 `VIN`**（板载 LDO 输入侧）。禁止 5 V。蓝板由 MicroUSB 独立供电，见 [供电与共地](../../../hardware/power-and-common-ground.md)。

## 上电 boot（约 20 ms）

DocID026899 Rev 8 正文未单独给出毫秒级 boot 延时；**AN4650**（应用笔记）写明：

> After the power supply is applied, the LSM6DS3 performs a **20 ms** boot procedure to load the trimming parameters. After the boot is completed, both the accelerometer and the gyroscope are automatically configured in Power-Down mode.

实现要求：`SPI1_Init` / 上电之后、**首次访问寄存器之前**软件延时 ≥ 20 ms。

## SPI 从机时序上限（Mode 3 图表）

Table 6（PDF p.25），表征结果，生产未逐项测试：

| 符号 | 含义 | Min | Max | 单位 |
|------|------|-----|-----|------|
| `tc(SPC)` | SPI 时钟周期 | 100 | — | ns |
| `fc(SPC)` | SPI 时钟频率 | — | **10** | MHz |
| `tsu(CS)` | CS setup | 5 | — | ns |
| `th(CS)` | CS hold | 20 | — | ns |
| `tsu(SI)` | SDI setup | 5 | — | ns |
| `th(SI)` | SDI hold | 15 | — | ns |
| `tv(SO)` | SDO valid | — | 50 | ns |
| `th(SO)` | SDO hold | 5 | — | ns |
| `tdis(SO)` | SDO disable | — | 50 | ns |

F103 SPI1 在 APB2=72 MHz、分频 **DIV16** 时时钟约 **4.5 MHz**，低于 10 MHz 上限。

## 与 Mode 3 的对应关系

手册 Figure 3（PDF p.25）及 §6.2 描述：

- CS 为高时 **SPC 停在高电平**（空闲时钟高 → `CPOL=1`）
- 数据在 SPC **下降沿**驱动，在 **上升沿**采样（`CPHA=1`）

即本工程采用的 **SPI Mode 3**。MCU 主机须匹配该 CPOL/CPHA。
