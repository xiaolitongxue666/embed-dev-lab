/**
 * @file    nvic.h
 * @brief   Cortex-M3 NVIC 手写寄存器：使能 IRQ、设置优先级
 *
 * 不链接 CMSIS-Core。寄存器位于 PPB（0xE000xxxx），不是 STM32 外设总线。
 * USART1 外设 IRQn = 37（向量表索引 53 = 16 + 37）。
 * DMA1 CH4 IRQn = 14（USART1_TX）；CH5 IRQn = 15（USART1_RX）；CH6 IRQn = 16（I2C1_TX）。
 *
 * @see     doc/learn/interrupt-vector-table-and-nvic.md
 */

#ifndef NVIC_H
#define NVIC_H

/** USART1 全局中断号（Cortex-M 外设 IRQn，非向量表索引） */
#define USART1_IRQn 37U
/** DMA1 通道 4（USART1_TX） */
#define DMA1_Channel4_IRQn 14U
/** DMA1 通道 5（USART1_RX） */
#define DMA1_Channel5_IRQn 15U
/** DMA1 通道 6（I2C1_TX） */
#define DMA1_Channel6_IRQn 16U

/**
 * @brief  使能指定外设 IRQ（写 NVIC ISER）
 * @param  irqn  外设中断号（0 起；USART1 为 37）
 */
void NVIC_IRQ_Enable(unsigned int irqn);

/**
 * @brief  设置外设 IRQ 抢占优先级（写 NVIC IP，高 4 位有效）
 * @param  irqn  外设中断号
 * @param  prio  0–15，数值越小优先级越高
 */
void NVIC_IRQ_SetPriority(unsigned int irqn, unsigned int prio);

#endif /* NVIC_H */
