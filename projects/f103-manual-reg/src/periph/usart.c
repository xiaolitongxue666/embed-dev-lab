/**
 * @file    usart.c
 * @brief   USART1：DMA1 CH4 TX（IRQn 14）/ CH5 RX（IRQn 15）+ USART1 IDLE（IRQn 37）
 *
 * @target  STM32F103C8T6，USART1 默认引脚 PA9(TX) / PA10(RX)，挂 APB2 总线
 *
 * 时钟前提（见 system_stm32f1xx.c）：
 *   HSE 8 MHz × PLL×9 → SYSCLK 72 MHz
 *   APB2 不分频 → PCLK2 = 72 MHz（USART1 波特率分频时钟源）
 *
 * 波特率（RM0008 §27.3.1）：
 *   USARTDIV = 72_000_000 / (16 × 1_500_000) = 3 → BRR 写 0x0030
 *
 * DMA（RM0008，默认映射）：
 *   TX：DMA1 CH4，CR3.DMAT；RX：DMA1 CH5，CR3.DMAR，普通模式
 *   只开 CCR.TCIE；CCR.HTIE（传输过半中断）保持 0
 * 空闲（RM0008 §27）：CR1.IDLEIE；清 IDLE 须先读 SR 再读 DR；先关 CH5 再读 DR
 *
 * 硬件接线（CH341 USB-TTL）：
 *   模块 RX ← PA9；模块 TX → PA10；GND 共地
 *
 * printf 路径：syscalls.c 中 _write() → USART1_Write()；`\n` 在 _write 内补 `\r`
 *
 * @see     dma.c
 * @see     doc/projects/f103-manual-reg.md
 * @see     doc/learn/interrupt-vector-table-and-nvic.md
 */

#include "dma.h"
#include "nvic.h"
#include "usart.h"

#define RCC_BASE     0x40021000U
#define RCC_APB2ENR  (*(volatile unsigned int *)(RCC_BASE + 0x18U))

#define GPIOA_BASE   0x40010800U
#define GPIOA_CRH    (*(volatile unsigned int *)(GPIOA_BASE + 0x04U))

#define USART1_BASE  0x40013800U
#define USART1_SR    (*(volatile unsigned int *)(USART1_BASE + 0x00U))
#define USART1_DR    (*(volatile unsigned int *)(USART1_BASE + 0x04U))
#define USART1_BRR   (*(volatile unsigned int *)(USART1_BASE + 0x08U))
#define USART1_CR1   (*(volatile unsigned int *)(USART1_BASE + 0x0CU))
#define USART1_CR2   (*(volatile unsigned int *)(USART1_BASE + 0x10U))
#define USART1_CR3   (*(volatile unsigned int *)(USART1_BASE + 0x14U))

#define RCC_APB2ENR_IOPAEN   (1U << 2)
#define RCC_APB2ENR_USART1EN (1U << 14)

#define GPIOA_CRH_PA9_MASK   (0xFU << 4)
#define GPIOA_CRH_PA9_AF_PP  (0xBU << 4)
#define GPIOA_CRH_PA10_MASK      (0xFU << 8)
#define GPIOA_CRH_PA10_IN_FLOAT  (0x4U << 8)

#define USART_SR_ORE  (1U << 3)
#define USART_SR_IDLE (1U << 4)
#define USART_CR1_UE  (1U << 13)
#define USART_CR1_IDLEIE (1U << 4)
#define USART_CR1_TE  (1U << 3)
#define USART_CR1_RE  (1U << 2)
#define USART_CR3_DMAR (1U << 6)
#define USART_CR3_DMAT (1U << 7)

#define USART1_BRR_1500000 0x0030U
#define USART1_NVIC_PRIO 5U
#define DMA_NVIC_PRIO    6U

#define USART1_TX_BUF_SIZE 256U
#define USART1_RX_BUF_SIZE 256U

#define DMA1_ISR (*(volatile unsigned int *)(0x40020000U))
#define DMA_ISR_TCIF5 (1U << 17)

unsigned char g_usart1_tx_buf[USART1_TX_BUF_SIZE];
unsigned char g_usart1_rx_buf[USART1_RX_BUF_SIZE];
unsigned char g_usart1_rx_ready[USART1_RX_BUF_SIZE];
volatile unsigned int g_usart1_rx_ready_len;
volatile unsigned int g_usart1_tx_busy;

static USART1_RxHandle_t g_usart1_rx_handle;

#define USART1_DMA_CCR_RX (DMA_CCR_TCIE | DMA_CCR_MINC)
#define USART1_DMA_CCR_TX (DMA_CCR_TCIE | DMA_CCR_DIR | DMA_CCR_MINC)

static void usart1_rx_dma_arm(void)
{
    DMA1_Channel_Start(DMA1_CHANNEL5,
                       USART1_DMA_CCR_RX & ~DMA_CCR_HTIE,
                       USART1_BASE + 0x04U,
                       (unsigned int)g_usart1_rx_buf,
                       USART1_RX_BUF_SIZE);
}

static void usart1_rx_copy_and_rearm(void)
{
    unsigned int remaining;
    unsigned int len;
    unsigned int i;

    remaining = DMA1_Channel_Remaining(DMA1_CHANNEL5);
    if (remaining > USART1_RX_BUF_SIZE) {
        remaining = USART1_RX_BUF_SIZE;
    }
    len = USART1_RX_BUF_SIZE - remaining;
    if (len > 0U) {
        for (i = 0U; i < len; i++) {
            g_usart1_rx_ready[i] = g_usart1_rx_buf[i];
        }
        g_usart1_rx_ready_len = len;
    }
    DMA1_Channel_ClearFlags(DMA1_CHANNEL5);
    usart1_rx_dma_arm();
}

void USART1_Init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_USART1EN;
    DMA1_ClockEnable();

    GPIOA_CRH &= ~GPIOA_CRH_PA9_MASK;
    GPIOA_CRH |= GPIOA_CRH_PA9_AF_PP;
    GPIOA_CRH &= ~GPIOA_CRH_PA10_MASK;
    GPIOA_CRH |= GPIOA_CRH_PA10_IN_FLOAT;

    USART1_CR1 = 0U;
    USART1_BRR = USART1_BRR_1500000;
    USART1_CR2 = 0U;
    USART1_CR3 = USART_CR3_DMAR | USART_CR3_DMAT;
    USART1_CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_IDLEIE | USART_CR1_UE;

    NVIC_IRQ_SetPriority(USART1_IRQn, USART1_NVIC_PRIO);
    NVIC_IRQ_Enable(USART1_IRQn);
    NVIC_IRQ_SetPriority(DMA1_Channel4_IRQn, DMA_NVIC_PRIO);
    NVIC_IRQ_Enable(DMA1_Channel4_IRQn);
    NVIC_IRQ_SetPriority(DMA1_Channel5_IRQn, DMA_NVIC_PRIO);
    NVIC_IRQ_Enable(DMA1_Channel5_IRQn);

    usart1_rx_dma_arm();
}

void USART1_Write(const char *buf, int len)
{
    int offset;
    int chunk;
    int i;

    if (buf == 0 || len <= 0) {
        return;
    }

    offset = 0;
    while (offset < len) {
        chunk = len - offset;
        if (chunk > (int)USART1_TX_BUF_SIZE) {
            chunk = (int)USART1_TX_BUF_SIZE;
        }

        while (g_usart1_tx_busy != 0U) {
        }

        for (i = 0; i < chunk; i++) {
            g_usart1_tx_buf[i] = (unsigned char)buf[offset + i];
        }

        g_usart1_tx_busy = 1U;
        DMA1_Channel_Start(DMA1_CHANNEL4,
                           USART1_DMA_CCR_TX & ~DMA_CCR_HTIE,
                           USART1_BASE + 0x04U,
                           (unsigned int)g_usart1_tx_buf,
                           (unsigned int)chunk);

        while (g_usart1_tx_busy != 0U) {
        }

        offset += chunk;
    }
}

void USART1_SetRxHandle(USART1_RxHandle_t handle)
{
    g_usart1_rx_handle = handle;
}

void USART1_ProcessRx(void)
{
    USART1_RxHandle_t handle;
    unsigned int len;
    unsigned int i;
    unsigned char frame[USART1_RX_BUF_SIZE];

    handle = g_usart1_rx_handle;
    len = g_usart1_rx_ready_len;
    if (len == 0U) {
        return;
    }
    if (len > USART1_RX_BUF_SIZE) {
        len = USART1_RX_BUF_SIZE;
    }
    for (i = 0U; i < len; i++) {
        frame[i] = g_usart1_rx_ready[i];
    }
    g_usart1_rx_ready_len = 0U;
    if (handle != 0) {
        handle(frame, len);
    }
}

void USART1_IRQHandler(void)
{
    unsigned int sr;

    sr = USART1_SR;
    if ((sr & USART_SR_IDLE) != 0U) {
        DMA1_Channel_Stop(DMA1_CHANNEL5);
        (void)USART1_DR;
        usart1_rx_copy_and_rearm();
    } else if ((sr & USART_SR_ORE) != 0U) {
        (void)USART1_DR;
    }
}

void DMA1_Channel4_IRQHandler(void)
{
    DMA1_Channel_ClearFlags(DMA1_CHANNEL4);
    g_usart1_tx_busy = 0U;
}

void DMA1_Channel5_IRQHandler(void)
{
    if ((DMA1_ISR & DMA_ISR_TCIF5) != 0U) {
        DMA1_Channel_Stop(DMA1_CHANNEL5);
        (void)USART1_SR;
        (void)USART1_DR;
        usart1_rx_copy_and_rearm();
    } else {
        DMA1_Channel_ClearFlags(DMA1_CHANNEL5);
    }
}
