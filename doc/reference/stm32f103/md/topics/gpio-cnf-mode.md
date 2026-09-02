# GPIO CNF / MODE 编码（CRL / CRH）

| 字段 | 值 |
|------|-----|
| 来源 | RM0008 — §8 GPIOs and AFIOs |
| 版本 | Rev 9（本地 PDF）；编码与 Rev 21 一致 |
| PDF 页码 | §8 起约 p.138；CRL/CRH 配置表约 p.142（以本地 PDF 为准） |
| 学习笔记 | [gpio-eight-modes.md](../../../learn/gpio-eight-modes.md) |
| 源码 | [`usart.c`](../../../../projects/f103-manual-reg/src/usart.c) · [`spi.c`](../../../../projects/f103-manual-reg/src/spi.c) · [`main.c`](../../../../projects/f103-manual-reg/src/main.c) |

## 背景

STM32F103 每个 GPIO 脚在 `GPIOx_CRL`（脚 0–7）或 `GPIOx_CRH`（脚 8–15）中占 **4 bit**：

```text
CNFy[1:0] | MODEy[1:0]
```

写 `CRL`/`CRH` 前须使能对应端口时钟（`RCC_APB2ENR` 的 `IOP×EN`）。PC13 另须 Backup 域解锁，见 [backup-domain-pc13.md](backup-domain-pc13.md)。

## MODE[1:0]

| MODE | 含义 |
|------|------|
| `00` | 输入模式 |
| `01` | 输出模式，最大 10 MHz |
| `10` | 输出模式，最大 2 MHz |
| `11` | 输出模式，最大 50 MHz |

## CNF[1:0]（与 MODE 组合）

### 输入（MODE = `00`）

| CNF | 模式 |
|-----|------|
| `00` | 模拟输入 |
| `01` | 浮空输入 |
| `10` | 上拉 / 下拉输入（`ODR` = 1 → 上拉；`ODR` = 0 → 下拉） |
| `11` | 保留 |

### 输出（MODE ≠ `00`）

| CNF | 模式 |
|-----|------|
| `00` | 通用推挽输出 |
| `01` | 通用开漏输出 |
| `10` | 复用功能推挽输出 |
| `11` | 复用功能开漏输出 |

## 本仓库常用半字节

| 值 | CNF + MODE | 用途 |
|----|------------|------|
| `0x3` | 00 + 11 | PC13 / PA4 推挽 50 MHz |
| `0x4` | 01 + 00 | PA10 RX、PA6 MISO 浮空输入 |
| `0xB` | 10 + 11 | PA9 TX、PA5/PA7 SPI 复用推挽 50 MHz |

## 与学习文档

八种模式语义、浮空=高阻、HAL 对照：[gpio-eight-modes.md](../../../learn/gpio-eight-modes.md)。

引脚保护与 FT/TT：[gpio-protection-and-negative-voltage.md](../../../learn/gpio-protection-and-negative-voltage.md)。
