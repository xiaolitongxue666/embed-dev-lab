# STM32F103 GPIO 八种模式

整理自学习笔记，说明 F103 的 **8 种 GPIO 配置**、浮空输入与高阻态，以及与本仓库 [`f103-manual-reg`](../../projects/f103-manual-reg/) / [`f103-cmsis-hal`](../../projects/f103-cmsis-hal/) 的寄存器与 HAL 对照。CNF/MODE 位编码摘录见 [gpio-cnf-mode topic](../reference/stm32f103/md/topics/gpio-cnf-mode.md)；引脚保护与负压见 [gpio-protection-and-negative-voltage.md](gpio-protection-and-negative-voltage.md)。

---

## 一句话总结

F103 每个 GPIO 脚由 `CRL`/`CRH` 中 **4 bit（CNF[1:0] + MODE[1:0]）** 配置成 8 种状态之一：**推挽输出、开漏输出、复用推挽、复用开漏、浮空输入、上拉输入、下拉输入、模拟输入**。浮空输入 = 高阻态，电平由外部决定。

---

## 八种模式一览

| 模式 | 典型用途 | 本仓库例子 |
|------|----------|------------|
| **推挽输出** | LED、普通数字输出 | PC13 LED、SPI CS（PA4） |
| **开漏输出** | 需外部上拉的总线、电平转换 | （计划 I2C 亦可走复用开漏） |
| **复用推挽输出** | USART TX、SPI SCK/MOSI | PA9 USART1_TX、PA5/PA7 SPI1 |
| **复用开漏输出** | I2C SCL/SDA | 计划 PB6/PB7 I2C1 |
| **浮空输入** | USART RX、SPI MISO、外部已驱动的信号 | PA10 USART1_RX、PA6 SPI1_MISO |
| **上拉输入** | 按键接 GND、需默认高电平 | （本 demo 未用） |
| **下拉输入** | 按键接 VDD、需默认低电平 | （本 demo 未用） |
| **模拟输入** | ADC 通道 | （本 demo 未用） |

---

## 浮空输入 = 高阻态

**浮空输入（Input floating）**：片内既不接上拉也不接下拉，引脚对电源/地呈 **高阻**。

- 既不主动灌电流，也不主动拉电流
- 电平完全由 **外部电路** 决定
- 若引脚悬空（无外部驱动），读到的电平 **不定**，易受噪声干扰

USART RX、SPI MISO 常用浮空输入：对端（CH341 TX、传感器 MISO）会主动驱动高低电平，MCU 只需「听」。

---

## F103 配置编码（CNF + MODE）

每个脚占 `CRL`（引脚 0–7）或 `CRH`（引脚 8–15）中的 **4 bit**：

```text
位序（从高到低）：CNF1 CNF0 MODE1 MODE0
```

| MODE[1:0] | 含义 |
|-----------|------|
| `00` | **输入模式** |
| `01` | 输出，最大 10 MHz |
| `10` | 输出，最大 2 MHz |
| `11` | 输出，最大 50 MHz |

| MODE | CNF[1:0] | 模式 |
|------|----------|------|
| `00`（输入） | `00` | 模拟输入 |
| `00` | `01` | **浮空输入** |
| `00` | `10` | 上拉 / 下拉输入（由 `ODR` 选：1=上拉，0=下拉） |
| `00` | `11` | 保留 |
| 非 0（输出） | `00` | **通用推挽输出** |
| 非 0 | `01` | **通用开漏输出** |
| 非 0 | `10` | **复用推挽输出** |
| 非 0 | `11` | **复用开漏输出** |

常用半字节速查：

| 半字节 | 二进制 | 含义 |
|--------|--------|------|
| `0x4` | `0100` | 浮空输入（CNF=01, MODE=00） |
| `0x3` | `0011` | 推挽输出 50 MHz（CNF=00, MODE=11） |
| `0xB` | `1011` | 复用推挽 50 MHz（CNF=10, MODE=11） |
| `0x8` | `1000` | 上拉/下拉输入（CNF=10, MODE=00；再写 ODR） |

完整表与 RM0008 页码见 [gpio-cnf-mode.md](../reference/stm32f103/md/topics/gpio-cnf-mode.md)。

---

## HAL 对照（cmsis-hal）

| 手册概念 | HAL |
|----------|-----|
| 浮空输入 | `GPIO_MODE_INPUT` + `GPIO_NOPULL` |
| 上拉输入 | `GPIO_MODE_INPUT` + `GPIO_PULLUP` |
| 下拉输入 | `GPIO_MODE_INPUT` + `GPIO_PULLDOWN` |
| 推挽输出 | `GPIO_MODE_OUTPUT_PP` |
| 开漏输出 | `GPIO_MODE_OUTPUT_OD` |
| 复用推挽 | `GPIO_MODE_AF_PP` |
| 复用开漏 | `GPIO_MODE_AF_OD` |
| 模拟 | `GPIO_MODE_ANALOG` |

---

## 本仓库对照

### PC13 LED — 推挽输出

[`main.c`](../../projects/f103-manual-reg/src/main.c)：`GPIOC_CRH_PC13_OUT_PP`（CNF=00, MODE=11 → `0x3`）。须先 PWREN + DBP，见 [backup-domain-pc13](../reference/stm32f103/md/topics/backup-domain-pc13.md)。

HAL：[`MX_GPIO_Init`](../../projects/f103-cmsis-hal/src/main.c) 中 `GPIO_MODE_OUTPUT_PP`。

### USART1 — 复用推挽 TX + 浮空输入 RX

[`usart.c`](../../projects/f103-manual-reg/src/usart.c)：

- PA9：`0xB`（复用推挽 50 MHz）
- PA10：`0x4`（浮空输入）

HAL：[`HAL_UART_MspInit`](../../projects/f103-cmsis-hal/src/stm32f1xx_hal_msp.c) 中 `GPIO_MODE_AF_PP` / `GPIO_MODE_INPUT` + `GPIO_NOPULL`。

### SPI1 — 复用推挽 + 浮空 MISO + 推挽 CS

[`spi.c`](../../projects/f103-manual-reg/src/spi.c)：

- PA5/PA7：复用推挽 `0xB`
- PA6：浮空输入 `0x4`
- PA4 CS：推挽 `0x3`

### 计划 I2C — 复用开漏 + 上拉

见 [硬件外设与接线](../hardware/stm32f103-peripherals.md)：PB6/PB7 为 I2C1，开漏 + 模块上拉。

---

## 延伸阅读

| 主题 | 文档 |
|------|------|
| CNF/MODE 官方摘录 | [gpio-cnf-mode.md](../reference/stm32f103/md/topics/gpio-cnf-mode.md) |
| 引脚保护与负压 | [gpio-protection-and-negative-voltage.md](gpio-protection-and-negative-voltage.md) |
| MMIO 与 PC13 点灯 | [stm32f103-mmio-basics.md](stm32f103-mmio-basics.md) |
| 接线与引脚表 | [stm32f103-peripherals.md](../hardware/stm32f103-peripherals.md) |
| RM0008 §8 GPIO | [rm0008-index.md](../reference/stm32f103/md/rm0008-index.md) |
