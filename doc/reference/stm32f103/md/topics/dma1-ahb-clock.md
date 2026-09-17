# DMA1 时钟（AHB）

| 字段 | 值 |
|------|-----|
| 来源 | RM0008 RCC `AHBENR`、DMA 章 |
| 源码 | [`dma.c`](../../../../../projects/f103-manual-reg/src/periph/dma.c) 的 `DMA1_ClockEnable` |
| 调用 | [`usart.c`](../../../../../projects/f103-manual-reg/src/periph/usart.c) `USART1_Init`；[`i2c.c`](../../../../../projects/f103-manual-reg/src/periph/i2c.c) `I2C1_Init`；[`adc.c`](../../../../../projects/f103-manual-reg/src/periph/adc.c) |
| 通道 / IRQ | [dma1-irq-map.md](dma1-irq-map.md) |

DMA 是片上外设，**必须开时钟**。不是「纯硬件搬运、不用时钟」。

## 挂在 AHB，不是 APB

| 总线 | 本工程典型外设 |
|------|----------------|
| **AHB** | DMA1 |
| APB1 | I2C1、TIM2–7 |
| APB2 | GPIO、USART1、SPI1 |

本工程写 `RCC_AHBENR` bit0（`DMA1EN`）：

```c
RCC_AHBENR |= (1U << 0);   /* DMA1_ClockEnable() */
```

对照 StdPeriph `RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE)`。手写路径不链库。

开 DMA1 **总**时钟后，通道 1–7 都可用，**没有** Channel6 单独时钟开关。STM32F103C8T6 无 DMA2。

`USART1_Init` 与 `I2C1_Init` 都会调 `DMA1_ClockEnable`，重复置位无害。

## 与 I2C 是两套时钟

1. `RCC_APB1ENR.I2C1EN` — I2C1（APB1）
2. `RCC_AHBENR.DMA1EN` — DMA1（AHB）

缺 I2C 时钟：总线不起。缺 DMA1 时钟：命令轮询仍可能发出（关/开显示），**128 列搬不动**，屏上无图；DMA 寄存器读出来常为 0；TC 永不进 ISR。

页数据路径见 [I2C1 白话](i2c1-master-polling.md#白话谁走-dma中断在哪)。通道占用见 [dma1-irq-map.md](dma1-irq-map.md)。
