# Medium-density 存储器映射（F103C8）

| 字段 | 值 |
|------|-----|
| 来源 | DS5319 §4 Memory mapping；RM0008 §2.3 Memory map |
| PDF 页码 | DS5319 p.34；RM0008 p.41–44 |
| 整理日期 | 2026-06-11 |
| 源码 | [`projects/f103-manual-reg/linker/STM32F103C8_FLASH.ld`](../../../projects/f103-manual-reg/linker/STM32F103C8_FLASH.ld) |

## STM32F103C8T6 容量

| 存储器 | 容量 | 总线地址 |
|--------|------|----------|
| Flash | **64 Kbytes** | `0x0800_0000` – `0x0800_FFFF` |
| SRAM | **20 Kbytes** | `0x2000_0000` – `0x2000_4FFF` |

DS5319 Table 1 / Features：STM32F103**x8** = 64 KB Flash + 20 KB SRAM；**xB** = 128 KB Flash + 20 KB SRAM。

## 链接脚本（f103-manual-reg）

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

与 `f103-manual-reg` 相关的条目：

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

RM0008 §2.3.2（p.43）：SRAM 与 **外设** 均支持位带。`gpioc_bitband.h` 中 `PCout(n)` 通过 `GPIOC_ODR`（`0x4001100C`）的位带别名实现单 bit 读写。

## 核对表

| 检查项 | DS5319 / RM0008 | `STM32F103C8_FLASH.ld` / 源码 | 结果 |
|--------|-----------------|-------------------------|------|
| Flash 大小 | 64 KB (x8) | `LENGTH = 64K` | OK |
| SRAM 大小 | 20 KB | `LENGTH = 20K` | OK |
| Flash 基址 | `0x08000000` | `ORIGIN = 0x08000000` | OK |
| SRAM 基址 | `0x20000000` | `ORIGIN = 0x20000000` | OK |
| 栈顶 | RAM 高地址 | `_estack = 0x20005000` | OK |
| RCC 基址 | `0x40021000` | `system_stm32f1xx.c` | OK |

## Boot 引脚与启动重映射（RM0008 §2.3 Memory map / Boot configuration）

上电复位后，**逻辑地址 `0x0000_0000` 起**的 Code 区窗口由硬件**镜像映射**到下列物理基址之一（**地址转发，不复制数据**）：

| BOOT0 | BOOT1 | 逻辑 `0x0000_0000` 映射到 | 物理存储 |
|-------|-------|---------------------------|----------|
| 0 | X | `0x0800_0000` | Main Flash（用户固件） |
| 1 | 0 | `0x1FFFF000` | System memory（出厂 ISP Bootloader） |
| 1 | 1 | `0x2000_0000` | SRAM |

Cortex-M3 复位从向量表基址读 **+0 → MSP**、**+4 → PC**。Flash 启动时 CPU 访问逻辑 `0x0000_0000/+4`，与物理 `0x0800_0000/+4` 内容相同。链接脚本使用 **Flash 物理基址** `0x08000000`，与 probe-rs 烧录、map 文件一致。叙述见 [stm32f103-memory-boot-map.md](../../../../learn/stm32f103-memory-boot-map.md)。

## System 区 Flash/OTP（`0x1FFFFxxx`，非 Main Flash）

| 区域 | 地址（medium-density） | 容量 | 说明 |
|------|------------------------|------|------|
| System memory | `0x1FFFF000` – `0x1FFFF2FF` | **12 KB** | ST 出厂 ISP Bootloader ROM，用户不可擦写 |
| `FLASHSIZE` | `0x1FFFF7E0` | 2 B | 只读 Flash 容量（CMSIS `FLASHSIZE_BASE`） |
| Unique device ID | `0x1FFFF7E8` – `0x1FFFF7F7` | 12 B | 96 bit UID（CMSIS `UID_BASE`） |
| Option bytes | `0x1FFFF800` – `0x1FFFF80F` | 16 B | RDP、写保护、USER 等（CMSIS `OB_BASE`） |

> **注意**：System memory 基址为 **`0x1FFFF000`**（五位 `F`），与 Main Flash `0x08000000` 不连续。

## 链接脚本为何用 `0x08000000` 而非 `0x00000000`

| 视角 | 使用的地址 | 原因 |
|------|------------|------|
| CPU 复位（Flash 启动） | 逻辑 `0x00000000` | ARM 默认向量表基址 + ST 别名 |
| 链接脚本 / 烧录 / 调试 | 物理 `0x08000000` | Main Flash 永久映射；与 ELF、map 一致 |

## 外设区 vs PPB 区（RM0008 memory map）

| 地址区 | 内容 | 设计方 |
|--------|------|--------|
| `0x4000_0000` 起 | ST 片上外设（RCC、GPIO…） | ST |
| `0xE000_0000` 起 | PPB 内核私有（NVIC、SysTick、SCB/VTOR） | ARM |

MMIO 机制与 PC13 实例见 [stm32f103-mmio-basics.md](../../../../learn/stm32f103-mmio-basics.md)。

## 延伸阅读

- [stm32f103-mmio-basics.md](../../../../learn/stm32f103-mmio-basics.md) — MMIO、flip-flop、手册地址  
- [stm32f103-memory-boot-map.md](../../../../learn/stm32f103-memory-boot-map.md) — 启动流程、Flash 段布局、SoC 分层  
- [rm0008-index.md](../rm0008-index.md) — §2 Memory and bus architecture  
- [datasheet-index.md](../datasheet-index.md) — §4 Memory mapping
