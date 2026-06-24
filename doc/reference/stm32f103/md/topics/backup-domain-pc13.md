# Backup 域、PWR 与 PC13 GPIO

| 字段 | 值 |
|------|-----|
| 来源 | RM0008 — §4 Power control, §5 BKP, §8 GPIO |
| 版本 | Rev 9（本地 PDF）；寄存器与 Rev 21 一致 |
| PDF 页码 | 54–55, 62–65, 138, 142 |
| 整理日期 | 2026-06-11 |
| 源码 | [`projects/f103-manual-reg/src/main.c`](../../../projects/f103-manual-reg/src/main.c) |

## 背景

**PC13** 属于 **Battery backup domain**。在 VDD 供电时，PC13 可作 GPIO；但 Backup 域寄存器默认写保护，必须先解除 **DBP**，否则对 `GPIOC_CRH` 的写入无效（LED 不亮）。

RM0008 §4.1.2（PDF p.54–55）：

> The VBAT pin powers the RTC unit, the LSE oscillator and the **PC13 to PC15 IOs** …  
> **PC13** can be used as GPIO, TAMPER pin, RTC Calibration Clock, RTC Alarm or second output.

> Due to the fact that the switch only sinks a limited amount of current (3 mA), the use of GPIOs **PC13 to PC15 in output mode is restricted**: the speed has to be limited to **2 MHz** with a maximum load of 30 pF …

厂商例程与 `f103-manual-reg` 使用 **50 MHz 推挽** 在多数 C8 核心板上仍可工作；若需严格符合手册，可将 PC13 配置为 2 MHz 输出。

## 初始化顺序（f103-manual-reg）

```c
RCC_APB1ENR |= RCC_APB1ENR_PWREN;   // 1. 开启 PWR 时钟
PWR_CR      |= PWR_CR_DBP;          // 2. 解除 Backup 域写保护
RCC_APB2ENR |= RCC_APB2ENR_IOPCEN;  // 3. 开启 GPIOC 时钟
GPIOC_CRH    = ... PC13 推挽输出 ... // 4. 配置 PC13
```

## 寄存器与地址

| 符号 | 地址 / 偏移 | 位域 | 说明 |
|------|-------------|------|------|
| `RCC_BASE` | `0x40021000` | — | RCC 基地址（RM0008 Table 1, p.41） |
| `RCC_APB1ENR` | `RCC_BASE + 0x1C` | **bit 28 PWREN** | PWR 接口时钟使能（RM0008 §6.3.8, p.97） |
| `PWR_BASE` | `0x40007000` | — | PWR（Table 1, p.42） |
| `PWR_CR` | `PWR_BASE + 0x00` | **bit 8 DBP** | 1 = 允许写 RTC 与 Backup 寄存器（§4.4.1, p.62–63） |
| `GPIOC_BASE` | `0x40011000` | — | GPIO Port C（Table 1, p.41） |
| `GPIOC_CRH` | `GPIOC_BASE + 0x04` | **bit[23:20]** | PC13 模式：CNF=00, MODE=11 → 50 MHz 推挽（§8.1.8, p.142） |
| `RCC_APB2ENR` | `RCC_BASE + 0x18` | **bit 4 IOPCEN** | GPIOC 时钟使能（§6.3.7, p.95） |

### PWR_CR.DBP（PDF p.62–63）

| Bit | 名称 | 含义 |
|-----|------|------|
| 8 | **DBP** | 0 = 禁止写 RTC/Backup；**1 = 允许** |

复位后 DBP=0，Backup 域 GPIO 配置被硬件忽略。

### RCC_APB1ENR.PWREN（PDF p.97）

| Bit | 名称 | 含义 |
|-----|------|------|
| 28 | **PWREN** | 1 = 使能 PWR 模块时钟 |

## PC13 与 LED

多数 **STM32F103C8 核心板** 将 LED 接 PC13，**低电平点亮**：

- `PCout(13) = 0` → 亮  
- `PCout(13) = 1` → 灭  

位带宏见 [`gpioc_bitband.h`](../../../projects/f103-manual-reg/src/gpioc_bitband.h)（`GPIOC_ODR` @ `0x4001100C`）。

## 核对表（与 main.c 一致）

| 检查项 | 手册 | 源码 | 结果 |
|--------|------|------|------|
| `RCC_BASE` | `0x40021000` | `main.c` | OK |
| `PWR_BASE` | `0x40007000` | `main.c` | OK |
| `GPIOC_BASE` | `0x40011000` | `main.c` | OK |
| `RCC_APB1ENR` 偏移 | `+0x1C` | `+0x1CU` | OK |
| `PWREN` | bit 28 | `(1U << 28)` | OK |
| `DBP` | bit 8 | `(1U << 8)` | OK |
| `IOPCEN` | bit 4 | `(1U << 4)` | OK |
| PC13 在 CRH | bit[23:20] | `(0xFU << 20)` | OK |

## 延伸阅读

- [rm0008-index.md](../rm0008-index.md) — §4 PWR、§5 BKP、§8 GPIO  
- [f103-manual-reg 模块](../../../projects/f103-manual-reg.md) — 模块说明
