/**
 * @file    dma.h
 * @brief   DMA1 通道手写寄存器：启停、剩余计数、清标志
 *
 * 不链接 CMSIS。USART1_TX=CH4、USART1_RX=CH5（RM0008 默认映射）。
 *
 * @see     dma.c
 * @see     doc/learn/interrupt-vector-table-and-nvic.md
 */

#ifndef DMA_H
#define DMA_H

#define DMA1_CHANNEL4 4U
#define DMA1_CHANNEL5 5U

#define DMA_CCR_EN   (1U << 0)
#define DMA_CCR_TCIE (1U << 1)
#define DMA_CCR_HTIE (1U << 2) /**< 过半中断；USART DMA 须保持为 0 */
#define DMA_CCR_DIR  (1U << 4) /**< 1=内存→外设，0=外设→内存 */
#define DMA_CCR_MINC (1U << 7)

void DMA1_ClockEnable(void);
void DMA1_Channel_Stop(unsigned int channel);
void DMA1_Channel_ClearFlags(unsigned int channel);
unsigned int DMA1_Channel_Remaining(unsigned int channel);
void DMA1_Channel_Start(unsigned int channel, unsigned int ccr,
                        unsigned int periph_addr, unsigned int mem_addr,
                        unsigned int count);

#endif /* DMA_H */
