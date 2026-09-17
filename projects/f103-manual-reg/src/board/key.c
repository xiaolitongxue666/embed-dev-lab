/**
 * @file    key.c
 * @brief   PB13 → EXTI13 双沿；IRQn=40
 *
 * RM0008：AFIO_EXTICR4.EXTI13=PB；IMR/FTSR/RTSR 线 13。
 *
 * @see     key.h
 */

#include "key.h"
#include "nvic.h"

#define RCC_BASE     0x40021000U
#define RCC_APB2ENR  (*(volatile unsigned int *)(RCC_BASE + 0x18U))

#define AFIO_BASE    0x40010000U
#define AFIO_EXTICR4 (*(volatile unsigned int *)(AFIO_BASE + 0x14U))
#define EXTI_BASE    0x40010400U
#define EXTI_IMR     (*(volatile unsigned int *)(EXTI_BASE + 0x00U))
#define EXTI_RTSR    (*(volatile unsigned int *)(EXTI_BASE + 0x08U))
#define EXTI_FTSR    (*(volatile unsigned int *)(EXTI_BASE + 0x0CU))
#define EXTI_PR      (*(volatile unsigned int *)(EXTI_BASE + 0x14U))

#define RCC_APB2ENR_AFIOEN (1U << 0)
#define AFIO_EXTICR4_EXTI13_MASK (0xFU << 4)
#define AFIO_EXTICR4_EXTI13_PB   (1U << 4)
#define EXTI_LINE13        (1U << 13)
#define KEY_EXTI_PRIO      7U

static volatile unsigned char g_key_edge;

void Key_ExtiInit(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN;
    AFIO_EXTICR4 = (AFIO_EXTICR4 & ~AFIO_EXTICR4_EXTI13_MASK) |
                   AFIO_EXTICR4_EXTI13_PB;
    EXTI_FTSR |= EXTI_LINE13;
    EXTI_RTSR |= EXTI_LINE13;
    EXTI_IMR |= EXTI_LINE13;
    EXTI_PR = EXTI_LINE13;
    NVIC_IRQ_SetPriority(EXTI15_10_IRQn, KEY_EXTI_PRIO);
    NVIC_IRQ_Enable(EXTI15_10_IRQn);
}

unsigned char Key_TakeEdge(void)
{
    unsigned char edge;

    edge = g_key_edge;
    if (edge != 0U) {
        g_key_edge = 0U;
    }
    return edge;
}

void EXTI15_10_IRQHandler(void)
{
    if ((EXTI_PR & EXTI_LINE13) != 0U) {
        EXTI_PR = EXTI_LINE13;
        g_key_edge = 1U;
    }
}
