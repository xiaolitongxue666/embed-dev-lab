# Medium-density 存储器映射（F103C8）

| 字段 | 值 |
|------|-----|
| 来源 | DS5319 §4 Memory mapping；RM0008 §2.3 Memory map |
| PDF 页码 | DS5319 p.34；RM0008 p.41–44 |
| 整理日期 | 2026-06-11 |
| 源码 | [`modules/f103-blink/linker/stm32f103c8.ld`](../../../modules/f103-blink/linker/stm32f103c8.ld) |

## STM32F103C8T6 容量

| 存储器 | 容量 | 总线地址 |
|--------|------|----------|
| Flash | **64 Kbytes** | `0x0800_0000` – `0x0800_FFFF` |
| SRAM | **20 Kbytes** | `0x2000_0000` – `0x2000_4FFF` |

DS5319 Table 1 / Features：STM32F103**x8** = 64 KB Flash + 20 KB SRAM；**xB** = 128 KB Flash + 20 KB SRAM。

## 链接脚本（f103-blink）

```ld
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}
_estack = 0x20005000;   /* RAM 末尾，主栈顶 */
```

| 符号 | 值 | 说明 |
|------|-----|------|
| Flash ORIGIN | `0x08000000` | 向量表与代码起始 |
| Flash LENGTH | 64K | 与 C8 一致 |
| RAM ORIGIN | `0x20000000` | SRAM 起始 |
| RAM LENGTH | 20K | 与 C8 一致 |
| `_estack` | `0x20005000` | `0x20000000 + 20K` |

## 外设基地址（RM0008 Table 1, p.41–42）

与 `f103-blink` 相关的条目：

| 边界地址 | 外设 |
|----------|------|
| `0x4002_1000` – `0x4002_13FF` | RCC |
| `0x4002_2000` – `0x4002_23FF` | Flash 接口（`FLASH_ACR`） |
| `0x4001_1000` – `0x4001_13FF` | **GPIO Port C** |
| `0x4000_7000` – `0x4000_73FF` | **Power control (PWR)** |
| `0x4000_6C00` – `0x4000_6FFF` | Backup registers (BKP) |

代码中使用的简化基址：

- `RCC_BASE = 0x40021000`
- `GPIOC_BASE = 0x40011000`
- `PWR_BASE = 0x40007000`
- `FLASH_BASE = 0x40022000`

## 位带（Bit banding）

RM0008 §2.3.2（p.43）：SRAM 与 **外设** 均支持位带。`gpio_like51.h` 中 `PCout(n)` 通过 `GPIOC_ODR`（`0x4001100C`）的位带别名实现单 bit 读写。

## 核对表

| 检查项 | DS5319 / RM0008 | `stm32f103c8.ld` / 源码 | 结果 |
|--------|-----------------|-------------------------|------|
| Flash 大小 | 64 KB (x8) | `LENGTH = 64K` | OK |
| SRAM 大小 | 20 KB | `LENGTH = 20K` | OK |
| Flash 基址 | `0x08000000` | `ORIGIN = 0x08000000` | OK |
| SRAM 基址 | `0x20000000` | `ORIGIN = 0x20000000` | OK |
| 栈顶 | RAM 高地址 | `_estack = 0x20005000` | OK |
| RCC 基址 | `0x40021000` | `system_stm32f10x.c` | OK |

## 延伸阅读

- [rm0008-index.md](../rm0008-index.md) — §2 Memory and bus architecture  
- [datasheet-index.md](../datasheet-index.md) — §4 Memory mapping
