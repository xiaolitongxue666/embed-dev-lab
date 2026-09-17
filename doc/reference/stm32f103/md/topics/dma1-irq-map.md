# DMA1 请求与中断（C8T6）

| 字段 | 值 |
|------|-----|
| 来源 | RM0008 DMA 章：通道默认请求；NVIC 中断号见向量表 |
| 工程占用 | [`f103-manual-reg`](../../../projects/f103-manual-reg.md) § 中断与 DMA |
| 时钟 | [dma1-ahb-clock.md](dma1-ahb-clock.md) |
| 向量 / NVIC | [interrupt-vector-table-and-nvic.md](../../../../learn/interrupt-vector-table-and-nvic.md) |

STM32F103C8T6 **只有 DMA1**（7 通道），无 DMA2。通道时钟共用 `RCC_AHBENR.DMA1EN`。

## RM0008 默认请求（本工程用到的）

手册把多路外设挂到同一通道；本工程不改 `DMA_CSELR`（F103 无该寄存器），按复位默认映射：

| DMA1 CH | 本工程请求 | 同通道手册上还有（未用） |
|---------|------------|--------------------------|
| 1 | ADC1 | TIM2_CH3、TIM4_CH1 |
| 2 | SPI1_RX | USART3_TX、TIM1_CH1、TIM2_UP、TIM3_CH3 |
| 3 | SPI1_TX | USART3_RX、TIM1_CH2、TIM3_CH4、TIM3_UP |
| 4 | USART1_TX | SPI2_RX、I2C2_TX、TIM1_CH4/TRIG/COM、TIM4_CH2 |
| 5 | USART1_RX | SPI2_TX、I2C2_RX、TIM1_UP、TIM2_CH1、TIM4_CH3 |
| 6 | I2C1_TX | USART2_RX、TIM1_CH3、TIM3_CH1/TRIG |
| 7 | 空 | USART2_TX、I2C1_RX、TIM2_CH2/CH4、TIM4_UP |

对照 RM0008「DMA1 request mapping」表；页码随 PDF 版次，入口见 [rm0008-index.md](../rm0008-index.md)。

## 本工程占用

| DMA1 CH | 模式 | IRQ | 源文件 |
|---------|------|-----|--------|
| 1 | 循环 16-bit，无 TCIE | 无 | [`periph/adc.c`](../../../../../projects/f103-manual-reg/src/periph/adc.c) |
| 2 | 普通，`WaitTc` | 无 | [`periph/spi.c`](../../../../../projects/f103-manual-reg/src/periph/spi.c) |
| 3 | 同上 | 无 | [`periph/spi.c`](../../../../../projects/f103-manual-reg/src/periph/spi.c) |
| 4 | 普通，TCIE，关 HTIE | IRQn 14 | [`periph/usart.c`](../../../../../projects/f103-manual-reg/src/periph/usart.c) |
| 5 | 普通 + USART IDLE 定界 | IRQn 15 + USART1 37 | [`periph/usart.c`](../../../../../projects/f103-manual-reg/src/periph/usart.c) |
| 6 | 普通，TCIE，关 HTIE | IRQn 16 | [`periph/i2c.c`](../../../../../projects/f103-manual-reg/src/periph/i2c.c) |
| 7 | 空 | — | — |

未开：I2C EV/ER、SPI IRQ、ADC EOC IRQ、TIM2 IRQ、DMA1 CH1/2/3/7 IRQ。

其它 IRQ：SysTick（异常 15）；EXTI15_10 IRQn 40（线 13=PB13），见 [`board/key.c`](../../../../../projects/f103-manual-reg/src/board/key.c)。
