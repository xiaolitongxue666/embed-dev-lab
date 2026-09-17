/**
 * @file    dma.c
 * @brief   DMA1 通道 MMIO：AHBENR.DMA1EN、CCR/CNDTR/CPAR/CMAR、ISR/IFCR
 *
 * 通道 n 寄存器块：0x40020008 + (n-1)×0x14（RM0008）。
 * CH1=ADC1，CH2=SPI1_RX，CH3=SPI1_TX，
 * CH4=USART1_TX，CH5=USART1_RX，CH6=I2C1_TX。
 * 调用方须清 CCR.HTIE，只开 TCIE。
 *
 * DMA1 在 AHB：DMA1_ClockEnable 写 AHBENR.DMA1EN。C8T6 无 DMA2。
 *
 * @see     doc/reference/stm32f103/md/topics/dma1-ahb-clock.md
 */

#include "dma.h"

#define RCC_BASE     0x40021000U
#define RCC_AHBENR   (*(volatile unsigned int *)(RCC_BASE + 0x14U))
#define RCC_AHBENR_DMA1EN (1U << 0)

#define DMA1_BASE         0x40020000U
#define DMA1_ISR          (*(volatile unsigned int *)(DMA1_BASE + 0x00U))
#define DMA1_IFCR         (*(volatile unsigned int *)(DMA1_BASE + 0x04U))
#define DMA1_CH1_BASE     0x40020008U
#define DMA1_CHANNEL_STRIDE 0x14U

#define DMA1_CHANNEL_MIN 1U
#define DMA1_CHANNEL_MAX 7U

static volatile unsigned int *dma1_reg(unsigned int channel, unsigned int offset)
{
    unsigned int base;

    base = DMA1_CH1_BASE + ((channel - 1U) * DMA1_CHANNEL_STRIDE);
    return (volatile unsigned int *)(base + offset);
}

static unsigned int dma1_flag_shift(unsigned int channel)
{
    return (channel - 1U) * 4U;
}

void DMA1_ClockEnable(void)
{
    RCC_AHBENR |= RCC_AHBENR_DMA1EN;
}

void DMA1_Channel_Stop(unsigned int channel)
{
    volatile unsigned int *ccr;

    if ((channel < DMA1_CHANNEL_MIN) || (channel > DMA1_CHANNEL_MAX)) {
        return;
    }
    ccr = dma1_reg(channel, 0U);
    *ccr &= ~DMA_CCR_EN;
}

void DMA1_Channel_ClearFlags(unsigned int channel)
{
    if ((channel < DMA1_CHANNEL_MIN) || (channel > DMA1_CHANNEL_MAX)) {
        return;
    }
    DMA1_IFCR = (0xFU << dma1_flag_shift(channel));
    (void)DMA1_ISR;
}

unsigned int DMA1_Channel_Remaining(unsigned int channel)
{
    if ((channel < DMA1_CHANNEL_MIN) || (channel > DMA1_CHANNEL_MAX)) {
        return 0U;
    }
    return *dma1_reg(channel, 4U);
}

void DMA1_Channel_Start(unsigned int channel, unsigned int ccr,
                        unsigned int periph_addr, unsigned int mem_addr,
                        unsigned int count)
{
    volatile unsigned int *ccr_reg;

    if ((channel < DMA1_CHANNEL_MIN) || (channel > DMA1_CHANNEL_MAX) ||
        (count == 0U)) {
        return;
    }

    DMA1_Channel_Stop(channel);
    DMA1_Channel_ClearFlags(channel);

    *dma1_reg(channel, 8U) = periph_addr;
    *dma1_reg(channel, 12U) = mem_addr;
    *dma1_reg(channel, 4U) = count;

    ccr_reg = dma1_reg(channel, 0U);
    *ccr_reg = (ccr & ~DMA_CCR_EN) | DMA_CCR_EN;
}

unsigned int DMA1_Channel_WaitTc(unsigned int channel, unsigned int loops)
{
    unsigned int tcif;

    if ((channel < DMA1_CHANNEL_MIN) || (channel > DMA1_CHANNEL_MAX)) {
        return 0U;
    }

    tcif = 1U << (dma1_flag_shift(channel) + 1U);
    while (loops != 0U) {
        if ((DMA1_ISR & tcif) != 0U) {
            return 1U;
        }
        loops--;
    }
    return 0U;
}
