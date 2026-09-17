/**
 * @file    tim2.c
 * @brief   TIM2 通道 2 PWM（PA1）驱动 JY003
 *
 * 寄存器偏移与位域对齐 ST CMSIS Device `stm32f103xb.h` / RM0008 通用定时器：
 *   TIM2 @ 0x40000000（APB1）
 *   CR1 CEN=bit0、ARPE=bit7
 *   CCMR1 OC2PE=bit11、OC2M PWM1=110（bit14:12）
 *   CCER CC2E=bit4、CC2P=0 高有效
 *   EGR UG=bit0 装载 PSC/ARR
 *   RCC_APB1ENR TIM2EN=bit0
 *
 * PWM mode 1：CNT < CCR2 时通道有效（高）。
 * GPIO：PA1 复用推挽 50 MHz（CNF=10 MODE=11 → 0xB），见 gpio-cnf-mode.md。
 * 不改 AFIO_MAPR，保持 TIM2 默认映射 CH2=PA1。未开 TIM2 IRQ。
 *
 * @see     tim2.h
 * @see     doc/projects/f103-manual-reg.md
 */

#include "tim2.h"

#define RCC_BASE     0x40021000U
#define RCC_APB2ENR  (*(volatile unsigned int *)(RCC_BASE + 0x18U))
#define RCC_APB1ENR  (*(volatile unsigned int *)(RCC_BASE + 0x1CU))

#define GPIOA_BASE   0x40010800U
#define GPIOA_CRL    (*(volatile unsigned int *)(GPIOA_BASE + 0x00U))

#define TIM2_BASE    0x40000000U
#define TIM2_CR1     (*(volatile unsigned int *)(TIM2_BASE + 0x00U))
#define TIM2_EGR     (*(volatile unsigned int *)(TIM2_BASE + 0x14U))
#define TIM2_CCMR1   (*(volatile unsigned int *)(TIM2_BASE + 0x18U))
#define TIM2_CCER    (*(volatile unsigned int *)(TIM2_BASE + 0x20U))
#define TIM2_PSC     (*(volatile unsigned int *)(TIM2_BASE + 0x28U))
#define TIM2_ARR     (*(volatile unsigned int *)(TIM2_BASE + 0x2CU))
#define TIM2_CCR2    (*(volatile unsigned int *)(TIM2_BASE + 0x38U))

#define RCC_APB2ENR_IOPAEN (1U << 2)
#define RCC_APB1ENR_TIM2EN (1U << 0)

#define GPIOA_CRL_PA1_MASK   (0xFU << 4)
#define GPIOA_CRL_PA1_AF_PP  (0xBU << 4)

#define TIM_CR1_CEN          (1U << 0)
#define TIM_CR1_ARPE         (1U << 7)
#define TIM_EGR_UG           (1U << 0)
#define TIM_CCMR1_OC2PE      (1U << 11)
#define TIM_CCMR1_OC2M_PWM1  (6U << 12)
#define TIM_CCER_CC2E        (1U << 4)

/** 72 MHz / (2+1) / (999+1) = 24 kHz */
#define TIM2_PWM_PSC 2U
#define TIM2_PWM_ARR FAN_DUTY_MAX

void TIM2_PWM_Init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN;
    RCC_APB1ENR |= RCC_APB1ENR_TIM2EN;

    GPIOA_CRL = (GPIOA_CRL & ~GPIOA_CRL_PA1_MASK) | GPIOA_CRL_PA1_AF_PP;

    TIM2_PSC = TIM2_PWM_PSC;
    TIM2_ARR = TIM2_PWM_ARR;
    TIM2_CCR2 = 0U;

    TIM2_CCMR1 = TIM_CCMR1_OC2M_PWM1 | TIM_CCMR1_OC2PE;
    TIM2_CCER |= TIM_CCER_CC2E;
    TIM2_CR1 |= TIM_CR1_ARPE;
    TIM2_EGR = TIM_EGR_UG;
    TIM2_CR1 |= TIM_CR1_CEN;
}

void Fan_SetDuty(unsigned int duty)
{
    if (duty > FAN_DUTY_MAX) {
        duty = FAN_DUTY_MAX;
    }
    TIM2_CCR2 = duty;
}
