/**
 * @file    nvic.c
 * @brief   Cortex-M3 NVIC：ISER / IP 手写 MMIO
 *
 * ISER 基址 0xE000E100：每位对应一个 IRQn；USART1(37) 在 ISER[1] bit5。
 * IP 基址 0xE000E400：每 IRQ 一字节；F1 实现 4 bit 优先级，写在高半字节。
 *
 * @see     nvic.h
 * @see     doc/learn/interrupt-vector-table-and-nvic.md
 */

#include "nvic.h"

#define NVIC_ISER_BASE 0xE000E100U
#define NVIC_IP_BASE   0xE000E400U

#define NVIC_PRIO_SHIFT 4U
#define NVIC_PRIO_MASK  0x0FU

void NVIC_IRQ_Enable(unsigned int irqn)
{
    volatile unsigned int *iser;

    iser = (volatile unsigned int *)(NVIC_ISER_BASE + ((irqn / 32U) * 4U));
    *iser = (1U << (irqn % 32U));
}

void NVIC_IRQ_SetPriority(unsigned int irqn, unsigned int prio)
{
    volatile unsigned char *ip;

    ip = (volatile unsigned char *)(NVIC_IP_BASE + irqn);
    *ip = (unsigned char)((prio & NVIC_PRIO_MASK) << NVIC_PRIO_SHIFT);
}
