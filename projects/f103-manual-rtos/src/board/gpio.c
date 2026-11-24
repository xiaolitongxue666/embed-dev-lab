/**
 * @file    gpio.c
 * @brief   PC13 推挽（Backup 域）、PB12 推挽、PB13 上拉
 *
 * @see     gpio.h
 * @see     doc/reference/stm32f103/md/topics/backup-domain-pc13.md
 */

#include "gpio.h"
#include "gpioc_bitband.h"

#define RCC_BASE     0x40021000U
#define RCC_APB2ENR  (*(volatile unsigned int *)(RCC_BASE + 0x18U))
#define RCC_APB1ENR  (*(volatile unsigned int *)(RCC_BASE + 0x1CU))

#define PWR_BASE 0x40007000U
#define PWR_CR   (*(volatile unsigned int *)(PWR_BASE + 0x00U))

#define GPIOC_BASE 0x40011000U
#define GPIOC_CRH  (*(volatile unsigned int *)(GPIOC_BASE + 0x04U))
#define GPIOB_CRH  (*(volatile unsigned int *)(GPIOB_BASE + 0x04U))

#define RCC_APB1ENR_PWREN  (1U << 28)
#define RCC_APB2ENR_IOPBEN (1U << 3)
#define RCC_APB2ENR_IOPCEN (1U << 4)
#define PWR_CR_DBP         (1U << 8)

#define GPIOC_CRH_PC13_MASK   (0xFU << 20)
#define GPIOC_CRH_PC13_OUT_PP (3U << 20)
#define GPIOB_CRH_PB12_MASK   (0xFU << 16)
#define GPIOB_CRH_PB12_OUT_PP (3U << 16)
#define GPIOB_CRH_PB13_MASK   (0xFU << 20)
#define GPIOB_CRH_PB13_IN_PU  (8U << 20)

void GPIOC_Init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_PWREN;
    PWR_CR |= PWR_CR_DBP;

    RCC_APB2ENR |= RCC_APB2ENR_IOPCEN;

    GPIOC_CRH &= ~GPIOC_CRH_PC13_MASK;
    GPIOC_CRH |= GPIOC_CRH_PC13_OUT_PP;
}

void GPIOB_Init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPBEN;

    GPIOB_CRH &= ~GPIOB_CRH_PB12_MASK;
    GPIOB_CRH |= GPIOB_CRH_PB12_OUT_PP;

    /* CNF=10 MODE=00 → 上拉/下拉输入；ODR=1 选片内上拉 */
    GPIOB_CRH &= ~GPIOB_CRH_PB13_MASK;
    GPIOB_CRH |= GPIOB_CRH_PB13_IN_PU;
    PBout(KEY_PIN) = 1U;
}
