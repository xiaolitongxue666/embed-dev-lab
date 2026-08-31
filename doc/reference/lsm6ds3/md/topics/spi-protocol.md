# SPI 总线协议（4 线）

| 字段 | 值 |
|------|-----|
| 来源 | DocID026899 §6 Digital interfaces、§6.2 SPI bus interface |
| 版本 | Rev 8 |
| PDF 页码 | 34–39 |
| 整理日期 | 2026-08-31 |
| 源码 | [`projects/f103-manual-reg/src/lsm6ds3.c`](../../../../projects/f103-manual-reg/src/lsm6ds3.c) |

## 引脚与模式选择

Table 9（PDF p.34）：I2C 与 SPI 共用引脚。要使用 **I2C** 时须把 **CS 接高**（接 `Vdd_IO`）。本 demo 使用 **SPI**，由 MCU 软件驱动 CS。

| 引脚名 | SPI 功能 | 模块常见丝印 |
|--------|----------|--------------|
| `CS` | 片选；0=SPI 通信，1=SPI 空闲（可开 I2C） | `CS` |
| `SCL/SPC` | SPI 时钟 SPC | `SCL` |
| `SDA/SDI/SDO` | SPI 数据输入 SDI（3 线时兼 SDO） | `SDA` |
| `SDO/SA0` | SPI 数据输出 SDO（4 线必接） | `SAO` |

本工程 **4 线全双工**（模块丝印 → F103）：

| 模块丝印 | MCU | SPI 功能 |
|----------|-----|----------|
| `3V3` | 3.3 V | 供电（勿接 `VIN`） |
| `GND` | GND | 共地 |
| `SCL` | PA5 | SCK / SPC |
| `SDA` | PA7 | MOSI / SDI |
| `SAO` | PA6 | MISO / SDO |
| `CS` | PA4 | 片选（软件 GPIO） |

`SCK`/`MOSI`/`MISO`/`CS` 是板级信号名。本仓库 `spi.c` 用 **硬件 SPI1**（写 `CR1`/`SR`/`DR`，由片内外设打时钟移位），不是 GPIO 一位一位模拟时序的软 SPI；二者物理线相同，软件入口不同。分层说明见 [`spi.c`](../../../../projects/f103-manual-reg/src/spi.c) 与 [f103-manual-reg.md § SPI1](../../../projects/f103-manual-reg.md#软-spi-vs-硬件-spi本工程)。

完整板级说明见 [stm32f103-peripherals.md](../../../hardware/stm32f103-peripherals.md)；源码头注释见 [`spi.c`](../../../../projects/f103-manual-reg/src/spi.c)。

## CS 规则

- 传输开始：主机拉低 CS  
- 传输结束：主机拉高 CS  
- 空闲：CS 必须保持高；SPC 在 CS 为高时停在高电平  

整帧（含地址字节与全部数据字节）期间 CS 保持为低；多字节连续读/写同一次 CS 断言完成。

## 帧格式（MSB first）

读写均以 **16 个时钟脉冲**完成单寄存器访问；多字节则再追加每字节 8 个脉冲（§6.2，PDF p.36–38）。

| 位 | 含义 |
|----|------|
| bit 0（首发位） | `RW`：`1`=读，`0`=写 |
| bit 1–7 | 寄存器地址 `AD(6:0)` |
| bit 8–15 | 写：主机发出的 `DI(7:0)`；读：从机返回的 `DO(7:0)` |

软件常用编码：

- 读：第一字节 = `0x80 | (addr & 0x7F)`，第二字节发哑元、收数据  
- 写：第一字节 = `addr & 0x7F`，第二字节 = 数据  

## 多字节与 `IF_INC`

`CTRL3_C (12h)` 的 `IF_INC`（PDF §9.14）：

- 复位默认 **1**：多字节访问时地址自动递增  
- 为 0：多字节时地址保持不变  

本 demo 依赖复位默认值，从 `OUTX_L_G (22h)` 起连读 12 字节覆盖陀螺仪 + 加速度计输出。

## 3 线模式（本 demo 不用）

置 `CTRL3_C.SIM=1` 进入 3 线 SPI（SDI/SDO 复用同一线）。本工程保持 `SIM=0`（4 线）。
