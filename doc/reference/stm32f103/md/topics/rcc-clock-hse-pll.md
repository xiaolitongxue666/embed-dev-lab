# RCC：HSE → PLL → 72 MHz

| 字段 | 值 |
|------|-----|
| 来源 | RM0008 §6 RCC；DS5319 §2.3.7 / §5.3.6–5.3.8 |
| PDF 页码 | RM0008 p.74–86, 97–98；DS5319 p.15, 50, 56 |
| 整理日期 | 2026-06-11 |
| 源码 | [`modules/f103-blink/src/system_stm32f10x.c`](../../../modules/f103-blink/src/system_stm32f10x.c) |

## 目标配置（f103-blink）

| 项目 | 值 |
|------|-----|
| HSE | 8 MHz（核心板晶振） |
| PLL 源 | HSE（`PLLSRC=1`） |
| PLL 倍频 | ×9（`PLLMUL=0111`） |
| SYSCLK | 72 MHz |
| APB1 | HCLK/2 = 36 MHz（APB1 最高 36 MHz） |
| Flash 等待 | 2 wait states（64 MHz < SYSCLK ≤ 72 MHz） |

DS5319：medium-density 最高 **72 MHz**；无 HSE 或启动失败时保持 **HSI 8 MHz**（`set_sys_clock_to_72mhz` 带超时返回）。

## 流程（与 system_stm32f10x.c 对应）

1. **HSEON**，轮询 **HSERDY**（超时 `HSE_STARTUP_TIMEOUT`）
2. **FLASH_ACR**：预取使能 + **LATENCY=2**
3. **RCC_CFGR**：APB1 二分频（`PPRE1=100`）；`PLLSRC=HSE`；`PLLMUL=×9`
4. **PLLON**，等待 **PLLRDY**
5. **SW=PLL**，轮询 **SWS=PLL**

## 关键寄存器

| 寄存器 | 地址 | 位 / 域 | 含义 |
|--------|------|---------|------|
| `RCC_CR` | `0x40021000` | bit 16 HSEON | 外部 HSE 使能 |
| | | bit 17 HSERDY | HSE 就绪 |
| | | bit 24 PLLON | PLL 使能 |
| | | bit 25 PLLRDY | PLL 就绪 |
| `RCC_CFGR` | `0x40021004` | bit[1:0] SW | `10` = 选择 PLL 为 SYSCLK |
| | | bit[3:2] SWS | 读回当前时钟源 |
| | | bit[10:8] PPRE1 | `100` = APB1 = HCLK/2 |
| | | bit 16 PLLSRC | 1 = HSE 作为 PLL 输入 |
| | | bit[21:18] PLLMUL | `0111` = ×9 |
| `FLASH_ACR` | `0x40022000` | bit[2:0] LATENCY | `010` = 2 等待周期 |
| | | bit 4 PRFTBE | 预取缓冲使能 |

### PLLMUL（RM0008 §6.3.2, p.84–86）

| PLLMUL[3:0] | 倍频 |
|-------------|------|
| 0111 | PLL 输入 × **9** |

### PPRE1（同上）

| PPRE1[2:0] | APB1 |
|------------|------|
| 100 | HCLK / **2** |

## 代码常量核对

| 检查项 | 手册 | `system_stm32f10x.c` | 结果 |
|--------|------|----------------------|------|
| `RCC_BASE` | `0x40021000` | `0x40021000U` | OK |
| `FLASH_BASE` | `0x40022000` | `0x40022000U` | OK |
| HSEON | bit 16 | `(1U << 16)` | OK |
| PLLSRC HSE | bit 16 CFGR | `(1U << 16)` | OK |
| PLLMUL ×9 | 0111 @ [21:18] | `(7U << 18)` | OK |
| PPRE1 /2 | 100 @ [10:8] | `(4U << 8)` | OK |
| SW PLL | 10 @ [1:0] | `(2U << 0)` | OK |
| FLASH LATENCY2 | 2 @ [2:0] | `(2U << 0)` | OK |

## 延伸阅读

- [rm0008-index.md](../rm0008-index.md) — §6 RCC  
- [datasheet-index.md](../datasheet-index.md) — 时钟树、HSE/PLL 电气特性
