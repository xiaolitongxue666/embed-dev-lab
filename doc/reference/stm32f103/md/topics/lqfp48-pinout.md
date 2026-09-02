# LQFP48 引脚与复用（STM32F103C8）

| 字段 | 值 |
|------|-----|
| 来源 | DS5319 — §3 Pinouts and pin description，Table 5 |
| 版本 | Rev 20（本地 PDF）；电气 FT 定义同文档 §5 |
| PDF 页码 | Table 5 约 28–33/115；LQFP48 图约 26/115 |
| 重映射 | RM0008 Table 52–56（USART3/2/1、I2C1、SPI1） |
| 整理日期 | 2026-09-02 |
| 板级丝印总表 | [stm32f103c8t6-pinout.md](../../../../hardware/stm32f103c8t6-pinout.md) |
| FT / 保护 | [gpio-protection-and-negative-voltage.md](../../../../learn/gpio-protection-and-negative-voltage.md) |

## 范围

仅列 **LQFP48** 封装上存在的引脚（STM32F103C8T6）。Table 5 中 LQFP48 列为 `-` 的脚（如 PC0–PC12、多数 PD、全部 PE）**本封装无焊盘**，不列入下表。

ADC 通道写 **ADC12_INx**（ADC1/ADC2 共用）。**FT** = 手册 I/O Level 标 `FT`（5 V tolerant）；表中「—」表示手册该格为空（非 FT）。

## LQFP48 引脚定义（校正）

| 引脚 | FT | 复位后主功能 | 默认复用 | Remap 列（软件 remap） |
|------|----|--------------|----------|------------------------|
| PA0 | — | PA0 / WKUP | USART2_CTS / ADC12_IN0 / TIM2_CH1_ETR | — |
| PA1 | — | PA1 | USART2_RTS / ADC12_IN1 / TIM2_CH2 | — |
| PA2 | — | PA2 | USART2_TX / ADC12_IN2 / TIM2_CH3 | — |
| PA3 | — | PA3 | USART2_RX / ADC12_IN3 / TIM2_CH4 | — |
| PA4 | — | PA4 | SPI1_NSS / USART2_CK / ADC12_IN4 | — |
| PA5 | — | PA5 | SPI1_SCK / ADC12_IN5 | — |
| PA6 | — | PA6 | SPI1_MISO / ADC12_IN6 / TIM3_CH1 | TIM1_BKIN |
| PA7 | — | PA7 | SPI1_MOSI / ADC12_IN7 / TIM3_CH2 | TIM1_CH1N |
| PA8 | FT | PA8 | USART1_CK / TIM1_CH1 / MCO | — |
| PA9 | FT | PA9 | USART1_TX / TIM1_CH2 | — |
| PA10 | FT | PA10 | USART1_RX / TIM1_CH3 | — |
| PA11 | FT | PA11 | USART1_CTS / CAN_RX / TIM1_CH4 / USBDM | — |
| PA12 | FT | PA12 | USART1_RTS / TIM1_ETR / CAN_TX / USBDP | — |
| PA13 | FT | JTMS/SWDIO | — | （释放调试后可作 PA13） |
| PA14 | FT | JTCK/SWCLK | — | （释放调试后可作 PA14） |
| PA15 | FT | JTDI | — | TIM2_CH1_ETR / SPI1_NSS |
| PB0 | — | PB0 | ADC12_IN8 / TIM3_CH3 | TIM1_CH2N |
| PB1 | — | PB1 | ADC12_IN9 / TIM3_CH4 | TIM1_CH3N |
| PB2 | FT | PB2 / BOOT1 | — | — |
| PB3 | FT | JTDO | — | TIM2_CH2 / TRACESWO / SPI1_SCK |
| PB4 | FT | JNTRST | — | TIM3_CH1 / SPI1_MISO |
| PB5 | — | PB5 | I2C1_SMBAl / TIM3_CH2 / SPI1_MOSI | — |
| PB6 | FT | PB6 | I2C1_SCL / TIM4_CH1 | USART1_TX |
| PB7 | FT | PB7 | I2C1_SDA / TIM4_CH2 | USART1_RX |
| PB8 | FT | PB8 | TIM4_CH3 | I2C1_SCL / CAN_RX |
| PB9 | FT | PB9 | TIM4_CH4 | I2C1_SDA / CAN_TX |
| PB10 | FT | PB10 | I2C2_SCL / USART3_TX | TIM2_CH3 |
| PB11 | FT | PB11 | I2C2_SDA / USART3_RX | TIM2_CH4 |
| PB12 | FT | PB12 | SPI2_NSS / I2C2_SMBAl / USART3_CK / TIM1_BKIN | — |
| PB13 | FT | PB13 | SPI2_SCK / USART3_CTS / TIM1_CH1N | — |
| PB14 | FT | PB14 | SPI2_MISO / USART3_RTS / TIM1_CH2N | — |
| PB15 | FT | PB15 | SPI2_MOSI / TIM1_CH3N | — |
| PC13 | — | TAMPER-RTC | — | RTC 相关（见 Note 5/6） |
| PC14 | — | OSC32_IN | — | — |
| PC15 | — | OSC32_OUT | — | — |
| PD0 | FT | OSC_IN | — | （可 remap 出 PD0 / CAN_RX，见 Note 7） |
| PD1 | FT | OSC_OUT | — | （可 remap 出 PD1 / CAN_TX，见 Note 7） |
| NRST | — | NRST | — | — |
| BOOT0 | — | BOOT0 | — | — |
| VBAT | — | VBAT | — | — |
| VDD / VSS | — | 供电 | — | 多组电源脚 |
| VDDA / VSSA | — | 模拟供电 | — | — |

### 与常见网传表的差异（摘录）

| 网传说法 | DS5319 Table 5 |
|----------|----------------|
| PA0–PA7 / PB0 / PB1「5V 容忍」 | I/O Level **无 FT** |
| PB5「5V 容忍」 | **无 FT** |
| PA5 = TIM2_CH1 | **无** TIM2；仅 SPI1_SCK / ADC12_IN5 |
| PA6 = TIM1_CH1 | 默认 **TIM3_CH1**；Remap 为 TIM1_BKIN |
| PA7 = TIM1_CH1N（默认） | TIM1_CH1N 在 **Remap**；默认 TIM3_CH2 |
| ADC1_INx | 写 **ADC12_INx** |
| USART2/3「无重映射」 | 硅片有 remap；**LQFP48 无对应焊盘**（见下） |

## 电源与特殊脚（Note）

DS5319 Table 5 脚注（摘要）：

| Note | 要点 |
|------|------|
| **2** | `FT` = 5 V tolerant |
| **5** | PC13–PC15 经 Backup 开关供电，灌/吸电流受限（约 3 mA）；输出建议 ≤ **2 MHz**、负载 ≤ 30 pF；**不宜作电流源驱动 LED**（板载 LED 见 Backup topic） |
| **6** | PC13–PC15 主功能依赖 Backup 域内容，不完全随主复位恢复 |
| **7** | LQFP48 的 PD0/PD1 复位后为 **OSC_IN / OSC_OUT**；软件可 remap 出 PD0/PD1 GPIO（本仓库核心板接 8 MHz HSE，勿当普通 IO） |
| **9** | 带 `(9)` 的复用可经 `AFIO_MAPR` 重映射到其它脚（封装须有该脚） |

FT 电气：输入可到约 **VDD + 4.0 V**（须 VDD 已上电，且禁用内部上下拉）；**输出仍为 3.3 V 轨**；未上电勿灌 5 V。见 [gpio-protection-and-negative-voltage.md](../../../../learn/gpio-protection-and-negative-voltage.md)。

## 重映射与 LQFP48 可用性

| 外设 | 默认（本封装可用） | Remap | LQFP48 |
|------|-------------------|-------|--------|
| USART1 | TX=PA9，RX=PA10 | TX=PB6，RX=PB7 | 默认与 remap **均可** |
| USART2 | TX=PA2，RX=PA3（+ CTS/RTS/CK on PA0–PA4） | PD3–PD7 | Remap **仅 100/144 pin**；本封装只用默认 |
| USART3 | TX=PB10，RX=PB11 | partial PC10–PC12…；full PD8… | Remap 焊盘 **本封装无**；只用默认 |
| I2C1 | SCL=PB6，SDA=PB7 | SCL=PB8，SDA=PB9 | 均可（36-pin 无 remap） |
| SPI1 | NSS=PA4，SCK=PA5，MISO=PA6，MOSI=PA7 | PA15 / PB3 / PB4 / PB5 | 均可（须释放 JTAG 相关脚） |

本仓库约定：USART1 / SPI1 / I2C1 **均不 remap**，见 [stm32f103-peripherals.md](../../../../hardware/stm32f103-peripherals.md)。

## 调试口（SWJ）

复位后为 **SWJ-DP（JTAG + SWD）**，默认占用 **PA13、PA14、PA15、PB3、PB4**。  
仅开 Serial Wire 时仍占用 PA13/PA14。本仓库烧录用 probe-rs / ST-Link，**禁止把 PA13/PA14 改普通 GPIO / 串口**。见 [swd-vs-usart.md](../../../../learn/swd-vs-usart.md)。

## 启动脚

| 脚 | 说明 |
|----|------|
| BOOT0 | 专用输入；与 BOOT1(PB2) 决定启动映像，见 [stm32f103-memory-boot-map.md](../../../../learn/stm32f103-memory-boot-map.md) |
| PB2 | BOOT1；FT |

板级上拉/下拉以核心板原理图 / 跳帽为准，**不以网传「BOOT0 板上拉」写死**。

## 延伸阅读

| 主题 | 文档 |
|------|------|
| 核心板丝印总表与本仓库占用 | [stm32f103c8t6-pinout.md](../../../../hardware/stm32f103c8t6-pinout.md) |
| 外设接线与冲突 | [stm32f103-peripherals.md](../../../../hardware/stm32f103-peripherals.md) |
| Backup / PC13 | [backup-domain-pc13.md](backup-domain-pc13.md) |
| DS5319 目录 | [datasheet-index.md](../datasheet-index.md) |
