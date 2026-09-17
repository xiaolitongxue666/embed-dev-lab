/**
 * @file    adc.c
 * @brief   ADC1 CH0（PA0）MMIO：ADCPRE=/6、校准、CONT+DMA → DMA1 CH1
 *
 * ADC1 @ 0x40012400（RM0008）。规则组 CH0；CR2.CONT + CR2.DMA。
 * DMA1 CH1 循环写 g_adc1_raw，不开 TCIE（避免每样本进 ISR）。
 * ADC1_ReadRaw 只读 DMA 最近值，不再 SWSTART 空等。
 *
 * @see     adc.h
 * @see     doc/projects/f103-manual-reg.md
 */

#include "adc.h"
#include "dma.h"

#define RCC_BASE     0x40021000U
#define RCC_CFGR     (*(volatile unsigned int *)(RCC_BASE + 0x04U))
#define RCC_APB2ENR  (*(volatile unsigned int *)(RCC_BASE + 0x18U))

#define GPIOA_BASE   0x40010800U
#define GPIOA_CRL    (*(volatile unsigned int *)(GPIOA_BASE + 0x00U))

#define ADC1_BASE    0x40012400U
#define ADC1_CR2     (*(volatile unsigned int *)(ADC1_BASE + 0x08U))
#define ADC1_SMPR2   (*(volatile unsigned int *)(ADC1_BASE + 0x10U))
#define ADC1_SQR1    (*(volatile unsigned int *)(ADC1_BASE + 0x2CU))
#define ADC1_SQR3    (*(volatile unsigned int *)(ADC1_BASE + 0x34U))
#define ADC1_DR      (*(volatile unsigned int *)(ADC1_BASE + 0x4CU))

#define RCC_APB2ENR_IOPAEN  (1U << 2)
#define RCC_APB2ENR_ADC1EN  (1U << 9)
#define RCC_CFGR_ADCPRE_MASK (3U << 14)
#define RCC_CFGR_ADCPRE_DIV6 (2U << 14)

#define GPIOA_CRL_PA0_MASK   (0xFU << 0)
#define GPIOA_CRL_PA0_ANALOG (0x0U << 0)

#define ADC_CR2_ADON         (1U << 0)
#define ADC_CR2_CONT         (1U << 1)
#define ADC_CR2_CAL          (1U << 2)
#define ADC_CR2_RSTCAL       (1U << 3)
#define ADC_CR2_DMA          (1U << 8)
#define ADC_CR2_ALIGN        (1U << 11)
#define ADC_CR2_EXTSEL_SW    (7U << 17)
#define ADC_CR2_EXTTRIG      (1U << 20)
#define ADC_CR2_SWSTART      (1U << 22)

#define ADC_SMPR2_SMP0_MASK  (7U << 0)
#define ADC_SMPR2_SMP0_239P5 (7U << 0)
#define ADC_SQR1_L_MASK      (0xFU << 20)
#define ADC_SQR3_SQ1_MASK    (0x1FU << 0)

#define ADC1_DR_MASK         0xFFFU

#define ADC1_DMA_CCR (DMA_CCR_CIRC | DMA_CCR_PSIZE16 | DMA_CCR_MSIZE16)

static volatile unsigned short g_adc1_raw;

static void adc1_delay_tstab(void)
{
    volatile unsigned int count;

    count = 1000U;
    while (count != 0U) {
        count--;
    }
}

void ADC1_Init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_ADC1EN;

    RCC_CFGR = (RCC_CFGR & ~RCC_CFGR_ADCPRE_MASK) | RCC_CFGR_ADCPRE_DIV6;

    GPIOA_CRL &= ~GPIOA_CRL_PA0_MASK;
    GPIOA_CRL |= GPIOA_CRL_PA0_ANALOG;

    ADC1_CR2 |= ADC_CR2_ADON;
    adc1_delay_tstab();

    ADC1_CR2 |= ADC_CR2_RSTCAL;
    while ((ADC1_CR2 & ADC_CR2_RSTCAL) != 0U) {
    }

    ADC1_CR2 |= ADC_CR2_CAL;
    while ((ADC1_CR2 & ADC_CR2_CAL) != 0U) {
    }

    ADC1_SMPR2 = (ADC1_SMPR2 & ~ADC_SMPR2_SMP0_MASK) | ADC_SMPR2_SMP0_239P5;
    ADC1_SQR1 &= ~ADC_SQR1_L_MASK;
    ADC1_SQR3 = (ADC1_SQR3 & ~ADC_SQR3_SQ1_MASK);

    DMA1_ClockEnable();
    DMA1_Channel_Start(DMA1_CHANNEL1,
                       ADC1_DMA_CCR,
                       ADC1_BASE + 0x4CU,
                       (unsigned int)&g_adc1_raw,
                       1U);

    ADC1_CR2 &= ~ADC_CR2_ALIGN;
    ADC1_CR2 |= ADC_CR2_EXTSEL_SW | ADC_CR2_EXTTRIG |
                ADC_CR2_DMA | ADC_CR2_CONT | ADC_CR2_ADON;
    ADC1_CR2 |= ADC_CR2_SWSTART;
}

unsigned int ADC1_ReadRaw(void)
{
    return (unsigned int)g_adc1_raw & ADC1_DR_MASK;
}
