# STM32F103C8T6 核心板引脚总表

> 芯片：**STM32F103C8T6**，LQFP48；面向本仓库常用 **C8 核心板（蓝板）丝印**。  
> 复用与 FT 以 [DS5319 Table 5 摘录](../reference/stm32f103/md/topics/lqfp48-pinout.md) 为准，**不照抄**网传「全 GPIO 5V 容忍」。  
> 外设接线 / 采购见 [stm32f103-peripherals.md](stm32f103-peripherals.md)。

---

## 说明

1. **FT**：手册标 5 V tolerant 的脚；内核与输出仍为 **3.3 V**。仅 FT 脚在 VDD 已上电且内部上下拉关闭时，输入可到约 VDD+4 V；**不能输出 5 V**；未上电勿灌 5 V。详见 [gpio-protection-and-negative-voltage.md](../learn/gpio-protection-and-negative-voltage.md)。
2. **非 FT**：表中「—」；含 PA0–PA7、PB0/PB1、PB5、PC13–PC15 等。勿接 5 V 逻辑。
3. **SWD**：PA13=SWDIO、PA14=SWCLK；本仓库烧录保留，**禁止**改 GPIO/串口。见 [swd-vs-usart.md](../learn/swd-vs-usart.md)。
4. **本仓库占用** 列与 [stm32f103-peripherals.md](stm32f103-peripherals.md) §4 对齐。

---

## 引脚总表

| 丝印 | FT | 默认复用（摘录） | Remap / 备注 | 本仓库占用 |
|------|----|------------------|--------------|------------|
| PA0 | — | USART2_CTS / TIM2_CH1_ETR / ADC12_IN0 / WKUP | — | — |
| PA1 | — | USART2_RTS / TIM2_CH2 / ADC12_IN1 | — | — |
| PA2 | — | **USART2_TX** / TIM2_CH3 / ADC12_IN2 | 勿与 SPI1 同时用默认 USART2 | —（勿开 USART2） |
| PA3 | — | **USART2_RX** / TIM2_CH4 / ADC12_IN3 | 同上 | —（勿开 USART2） |
| PA4 | — | SPI1_NSS / USART2_CK / ADC12_IN4 | — | **SPI1 CS**（GPIO，已实现） |
| PA5 | — | SPI1_SCK / ADC12_IN5 | **无 TIM2_CH1** | **SPI1 SCK**（已实现） |
| PA6 | — | SPI1_MISO / TIM3_CH1 / ADC12_IN6 | Remap：TIM1_BKIN | **SPI1 MISO**（已实现） |
| PA7 | — | SPI1_MOSI / TIM3_CH2 / ADC12_IN7 | Remap：TIM1_CH1N | **SPI1 MOSI**（已实现） |
| PA8 | FT | USART1_CK / TIM1_CH1 / MCO | — | — |
| PA9 | FT | **USART1_TX** / TIM1_CH2 | Remap 到 PB6 | **USART1 TX**（已实现） |
| PA10 | FT | **USART1_RX** / TIM1_CH3 | Remap 到 PB7 | **USART1 RX**（已实现） |
| PA11 | FT | USART1_CTS / CAN_RX / TIM1_CH4 / **USBDM** | — | — |
| PA12 | FT | USART1_RTS / TIM1_ETR / CAN_TX / **USBDP** | — | — |
| PA13 | FT | **SWDIO**（JTMS） | 开启 SWD 后占用 | **SWD**（保留） |
| PA14 | FT | **SWCLK**（JTCK） | 同上 | **SWD**（保留） |
| PA15 | FT | JTDI | Remap：TIM2_CH1_ETR / SPI1_NSS | 复位属 JTAG；本仓库未用 |
| PB0 | — | TIM3_CH3 / ADC12_IN8 | Remap：TIM1_CH2N | FT6236 INT（预留，暂不使用） |
| PB1 | — | TIM3_CH4 / ADC12_IN9 | Remap：TIM1_CH3N | FT6236 RST（预留，暂不使用） |
| PB2 | FT | BOOT1 | 启动配置；尽量勿作普通 IO | — |
| PB3 | FT | JTDO | Remap：TIM2_CH2 / SPI1_SCK | 复位属 JTAG |
| PB4 | FT | JNTRST | Remap：TIM3_CH1 / SPI1_MISO | 复位属 JTAG |
| PB5 | — | I2C1_SMBAl / TIM3_CH2 / SPI1_MOSI | **非 FT** | — |
| PB6 | FT | **I2C1_SCL** / TIM4_CH1 | USART1_TX（remap） | **I2C1 SCL**（计划） |
| PB7 | FT | **I2C1_SDA** / TIM4_CH2 | USART1_RX（remap） | **I2C1 SDA**（计划） |
| PB8 | FT | TIM4_CH3 | I2C1_SCL / CAN_RX（remap） | — |
| PB9 | FT | TIM4_CH4 | I2C1_SDA / CAN_TX（remap） | — |
| PB10 | FT | **USART3_TX** / I2C2_SCL | TIM2_CH3（remap） | — |
| PB11 | FT | **USART3_RX** / I2C2_SDA | TIM2_CH4（remap） | — |
| PB12 | FT | SPI2_NSS / TIM1_BKIN / USART3_CK | — | — |
| PB13 | FT | SPI2_SCK / TIM1_CH1N / USART3_CTS | — | — |
| PB14 | FT | SPI2_MISO / TIM1_CH2N / USART3_RTS | — | — |
| PB15 | FT | SPI2_MOSI / TIM1_CH3N | — | — |
| PC13 | — | TAMPER / RTC | **非 FT**；Backup 域；板载 LED | **LED**（已实现，低电平点亮常见） |
| PC14 | — | OSC32_IN | 尽量勿作普通 IO | 板载 32.768 kHz（若有） |
| PC15 | — | OSC32_OUT | 同上 | 同上 |
| NRST | — | 复位（低有效） | — | 板载复位 |
| BOOT0 | — | 启动选择 | 以板跳帽为准；常见默认 Main Flash | — |
| VDD | — | 3.3 V | — | USB→板上 3.3 V |
| VSS | — | GND | — | 共地 |
| VBAT | — | RTC 后备 | 可接纽扣电池 | — |
| （芯片）PD0/PD1 | FT | OSC_IN / OSC_OUT | 8 MHz HSE；**多数板无排针** | HSE；勿当 IO |

完整 Default/Remap 与封装说明：[lqfp48-pinout.md](../reference/stm32f103/md/topics/lqfp48-pinout.md)。

---

## 1. USART 清单

| 串口 | 默认 TX / RX | Remap TX / RX | 本仓库 |
|------|--------------|---------------|--------|
| USART1 | PA9 / PA10 | PB6 / PB7 | **默认**；勿 remap（否则抢 I2C1） |
| USART2 | PA2 / PA3 | PD3–PD7（LQFP48 **无焊盘**） | **不用**（与 SPI1 冲突） |
| USART3 | PB10 / PB11 | PC/PD（LQFP48 **无焊盘**） | 未用 |

勿把 PA13/PA14 当串口。概念：[swd-vs-usart.md](../learn/swd-vs-usart.md)；电平：[uart-ttl-rs232-rs485.md](../learn/uart-ttl-rs232-rs485.md)。

CH341：RX←PA9，TX→PA10，GND→面包板 GND 轨。

---

## 2. SWD 调试

| 脚 | 功能 |
|----|------|
| PA13 | SWDIO |
| PA14 | SWCLK |

ST-Link：SWDIO→PA13，SWCLK→PA14；GND 与 3.3V 进面包板轨（方案 A：5V 闲置）；蓝板 SWD GND 从面包板轨短线接入。蓝板由 **MicroUSB 独立供电**；ST-Link 电源 **禁止**接到蓝板电源脚。见 [供电与共地](power-and-common-ground.md)。

复位后为 SWJ（另占 PA15/PB3/PB4）；本仓库只要求保留 PA13/PA14。

---

## 3. 板载固定资源

1. **PC13**：板载 LED（多数灌电流、低电平点亮）；非 FT；须 Backup 解锁；拉/灌约 ±3 mA，见 [backup-domain-pc13.md](../reference/stm32f103/md/topics/backup-domain-pc13.md) 与 [gpio-led-source-sink.md](../learn/gpio-led-source-sink.md)。
2. **PC14 / PC15**：LSE 32.768 kHz；不建议作普通 IO。
3. **PD0 / PD1（OSC）**：8 MHz HSE；芯片脚通常未引出排针。
4. **PB2 / BOOT0**：启动模式；尽量不作业务 IO。BOOT 表见 [stm32f103-memory-boot-map.md](../learn/stm32f103-memory-boot-map.md)。

---

## 4. 使用注意

1. 输出一律 3.3 V；仅 **FT** 脚可在手册条件下耐受较高输入，不能输出 5 V。
2. 串口优先默认脚；`USART1_REMAP` 与计划中的 I2C1（PB6/PB7）冲突。
3. SPI1 占用 PA4–PA7 时不要启用默认 USART2。
4. 本仓库外设模块统一 **3.3 V** 供电（见接线文）。

---

## 占用一览（与接线文一致）

```text
PC13              LED（已实现）
PA4–PA7           SPI1 → LSM6DS3（已实现，f103-manual-reg）
PA9, PA10         USART1（已实现）
PA13, PA14        SWD
PB0, PB1          FT6236 INT/RST（预留，暂不使用）
PB6, PB7          I2C1：SH1106 + 可选 BMP280（计划）
```

---

## 出处

| 资料 | 用途 |
|------|------|
| [lqfp48-pinout.md](../reference/stm32f103/md/topics/lqfp48-pinout.md) | DS5319 Table 5 / RM0008 remap 校正摘录 |
| [stm32f103-peripherals.md](stm32f103-peripherals.md) | 本仓库接线与冲突 |
| DS5319 / RM0008 | 官方 PDF：`doc/reference/stm32f103/pdf/`（`./scripts/fetch-stm32f103-docs.sh`） |
