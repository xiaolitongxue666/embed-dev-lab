/**
 * @file    dma.h
 * @brief   DMA1 通道手写寄存器：启停、剩余计数、清标志
 *
 * 不链接 CMSIS。RM0008 默认请求：
 *   CH1=ADC1，CH2=SPI1_RX，CH3=SPI1_TX，
 *   CH4=USART1_TX，CH5=USART1_RX，CH6=I2C1_TX。
 * DMA1 挂 AHB：须 DMA1_ClockEnable（RCC_AHBENR.DMA1EN）。通道无单独时钟。
 *
 * @see     dma.c
 * @see     doc/reference/stm32f103/md/topics/dma1-ahb-clock.md
 * @see     doc/reference/stm32f103/md/topics/dma1-irq-map.md
 * @see     doc/learn/interrupt-vector-table-and-nvic.md
 */

#ifndef DMA_H
#define DMA_H

#define DMA1_CHANNEL1 1U
#define DMA1_CHANNEL2 2U
#define DMA1_CHANNEL3 3U
#define DMA1_CHANNEL4 4U
#define DMA1_CHANNEL5 5U
#define DMA1_CHANNEL6 6U

#define DMA_CCR_EN      (1U << 0)
#define DMA_CCR_TCIE    (1U << 1)
#define DMA_CCR_HTIE    (1U << 2) /**< 过半中断；USART DMA 须保持为 0 */
#define DMA_CCR_DIR     (1U << 4) /**< 1=内存→外设，0=外设→内存 */
#define DMA_CCR_CIRC    (1U << 5)
#define DMA_CCR_MINC    (1U << 7)
#define DMA_CCR_PSIZE16 (1U << 8)
#define DMA_CCR_MSIZE16 (1U << 10)

/** 开 AHB 上 DMA1 总时钟（DMA1EN）；7 通道共用，无 CH 单独时钟 */
void DMA1_ClockEnable(void);
void DMA1_Channel_Stop(unsigned int channel);
void DMA1_Channel_ClearFlags(unsigned int channel);
unsigned int DMA1_Channel_Remaining(unsigned int channel);
void DMA1_Channel_Start(unsigned int channel, unsigned int ccr,
                        unsigned int periph_addr, unsigned int mem_addr,
                        unsigned int count);
/** 等 TCIFx，超时返回 0 */
unsigned int DMA1_Channel_WaitTc(unsigned int channel, unsigned int loops);

#endif /* DMA_H */
