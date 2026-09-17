/**
 * @file    i2c.c
 * @brief   I2C1 主机写：400 kHz；短包轮询，页数据 DMA1 CH6（IRQn 16）
 *
 * @target  STM32F103C8T6，I2C1 默认映射 PB6(SCL)/PB7(SDA)
 *
 * ---------------------------------------------------------------------------
 * 实现方式：硬件 I2C（不是 GPIO 模拟）
 *
 * 软 I2C / bit-bang：CPU 自己翻 SCL/SDA。
 * 本文件用片内 I2C1：软件写 CR1/CR2/CCR/DR/SR，硬件产生起停与移位。
 *
 * 引脚：
 *   PB6 → I2C1_SCL，PB7 → I2C1_SDA（复用开漏 50 MHz，半字节 0xF）
 *   PB6/PB7：DS5319 标 FT；本仓库模块 3.3 V
 *   模块板载上拉，外部不再加 4.7 kΩ
 *   I2C1 默认映射；USART1_REMAP 会抢 PB6/PB7 → 本工程不 remap
 *
 * ---------------------------------------------------------------------------
 * 两层命名（信号线 ≠ 寄存器）
 *
 * 【板级】SCL / SDA
 * 【MCU】CR1/CR2/CCR/TRISE/SR1/SR2/DR（RM0008 I2C 章，基址 0x40005400）
 *
 * 8 位写地址原样写入 DR。屏幕为 0x78（R6 焊、R5 空；0x78 == 0x3C<<1）。
 * 禁止再对 0x78 左移。StdPeriph I2C_Send7bitAddress(addr>>1) 的移位不要照抄。
 *
 * DMA（RM0008 默认映射）：I2C1_TX = DMA1 CH6。DMA 只搬内存→DR，
 * 不产生 START/STOP/从地址/控制字节。CR2.DMAEN 仅在页数据阶段打开。
 * USART 已用 CH4/CH5，勿占用。
 * I2C1 时钟在 APB1，DMA1 时钟在 AHB，两套都要开。
 * 页 DMA 完成：DMA1_Channel6_IRQHandler（本文件，非 HAL 回调）。
 *
 * OAR1 bit14 手册要求软件保持为 1。
 * F1 硬件 I2C 可能 BUSY 锁死：超时则 PE 关 + APB1 复位再恢复时序。
 *
 * 时钟前提（system_stm32f1xx.c）：
 *   SYSCLK 72 MHz，APB1 /2 → PCLK1 = 36 MHz（I2C1 时钟源）
 *   CR2.FREQ=36；Fast 400 kHz duty 2：CCR=30、FS=1；TRISE=11
 *
 * @see     dma.c
 * @see     doc/hardware/stm32f103-peripherals.md
 * @see     doc/reference/stm32f103/md/topics/i2c1-master-polling.md
 * @see     RM0008 I2C 章
 */

#include "dma.h"
#include "i2c.h"
#include "nvic.h"

/* -------------------------------------------------------------------------- */
/* 外设基地址与寄存器（RM0008）                                                 */
/* -------------------------------------------------------------------------- */

#define RCC_BASE       0x40021000U
#define RCC_APB2ENR    (*(volatile unsigned int *)(RCC_BASE + 0x18U))
#define RCC_APB1ENR    (*(volatile unsigned int *)(RCC_BASE + 0x1CU))
#define RCC_APB1RSTR   (*(volatile unsigned int *)(RCC_BASE + 0x10U))

#define GPIOB_BASE     0x40010C00U
#define GPIOB_CRL      (*(volatile unsigned int *)(GPIOB_BASE + 0x00U))

#define I2C1_BASE      0x40005400U
#define I2C1_CR1       (*(volatile unsigned int *)(I2C1_BASE + 0x00U))
#define I2C1_CR2       (*(volatile unsigned int *)(I2C1_BASE + 0x04U))
#define I2C1_OAR1      (*(volatile unsigned int *)(I2C1_BASE + 0x08U))
#define I2C1_DR        (*(volatile unsigned int *)(I2C1_BASE + 0x10U))
#define I2C1_SR1       (*(volatile unsigned int *)(I2C1_BASE + 0x14U))
#define I2C1_SR2       (*(volatile unsigned int *)(I2C1_BASE + 0x18U))
#define I2C1_CCR       (*(volatile unsigned int *)(I2C1_BASE + 0x1CU))
#define I2C1_TRISE     (*(volatile unsigned int *)(I2C1_BASE + 0x20U))

#define RCC_APB2ENR_IOPBEN  (1U << 3)
#define RCC_APB1ENR_I2C1EN  (1U << 21)
#define RCC_APB1RSTR_I2C1RST (1U << 21)

#define GPIOB_CRL_PB6_MASK   (0xFU << 24)
#define GPIOB_CRL_PB7_MASK   (0xFU << 28)
/** PB6/PB7 复用开漏 50 MHz：CNF=11 MODE=11 → 0xF */
#define GPIOB_CRL_PB6_AF_OD  (0xFU << 24)
#define GPIOB_CRL_PB7_AF_OD  (0xFU << 28)

#define I2C_CR1_PE     (1U << 0)
#define I2C_CR1_START  (1U << 8)
#define I2C_CR1_STOP   (1U << 9)
#define I2C_CR1_ACK    (1U << 10)

#define I2C_CR2_FREQ   36U
#define I2C_CR2_DMAEN  (1U << 11)

#define I2C_SR1_SB     (1U << 0)
#define I2C_SR1_ADDR   (1U << 1)
#define I2C_SR1_BTF    (1U << 2)
#define I2C_SR1_TXE    (1U << 7)
#define I2C_SR1_AF     (1U << 10)

#define I2C_SR2_BUSY   (1U << 1)

#define I2C_OAR1_BIT14 (1U << 14)

/** Fast 400 kHz、duty 2：CCR = 36e6 / (3 * 400e3) = 30；FS=1 */
#define I2C_CCR_FS     (1U << 15)
#define I2C_CCR_400KHZ 30U
/** Fast 最大上升 300 ns：TRISE = 36 * 300ns + 1 = 11 */
#define I2C_TRISE_FAST 11U

#define I2C_WAIT_LOOPS 200000U
#define I2C_DMA_NVIC_PRIO 6U
#define I2C1_DMA_CCR_TX (DMA_CCR_TCIE | DMA_CCR_DIR | DMA_CCR_MINC)

/** 1=空闲，0=CH6 正在搬 */
static volatile unsigned char i2c1_dma_tx_done = 1U;

static void i2c1_hw_apply(void)
{
    I2C1_CR1 &= ~I2C_CR1_PE;
    I2C1_CR2 = I2C_CR2_FREQ;
    I2C1_CCR = I2C_CCR_FS | I2C_CCR_400KHZ;
    I2C1_TRISE = I2C_TRISE_FAST;
    I2C1_OAR1 = I2C_OAR1_BIT14;
    I2C1_CR1 = I2C_CR1_ACK | I2C_CR1_PE;
}

/** F1 BUSY 锁死：关 PE、APB1 复位 I2C1，再写回时序（GPIO 不动） */
static void i2c1_recover_busy(void)
{
    I2C1_CR2 &= ~I2C_CR2_DMAEN;
    DMA1_Channel_Stop(DMA1_CHANNEL6);
    I2C1_CR1 &= ~I2C_CR1_PE;
    RCC_APB1RSTR |= RCC_APB1RSTR_I2C1RST;
    RCC_APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;
    i2c1_hw_apply();
    i2c1_dma_tx_done = 1U;
}

static unsigned char i2c1_wait_sr1(unsigned int mask)
{
    volatile unsigned int n;

    n = I2C_WAIT_LOOPS;
    while (n != 0U) {
        if ((I2C1_SR1 & I2C_SR1_AF) != 0U) {
            I2C1_SR1 &= ~I2C_SR1_AF;
            I2C1_CR1 |= I2C_CR1_STOP;
            return 0U;
        }
        if ((I2C1_SR1 & mask) != 0U) {
            return 1U;
        }
        n--;
    }
    I2C1_CR1 |= I2C_CR1_STOP;
    return 0U;
}

static unsigned char i2c1_wait_busy_clear(void)
{
    volatile unsigned int n;

    n = I2C_WAIT_LOOPS;
    while (n != 0U) {
        if ((I2C1_SR2 & I2C_SR2_BUSY) == 0U) {
            return 1U;
        }
        n--;
    }

    i2c1_recover_busy();
    n = I2C_WAIT_LOOPS;
    while (n != 0U) {
        if ((I2C1_SR2 & I2C_SR2_BUSY) == 0U) {
            return 1U;
        }
        n--;
    }
    I2C1_CR1 |= I2C_CR1_STOP;
    return 0U;
}

static unsigned char i2c1_start_addr(unsigned char addr8)
{
    unsigned int sr1;

    if (i2c1_wait_busy_clear() == 0U) {
        return 0U;
    }

    I2C1_CR1 |= I2C_CR1_START;
    if (i2c1_wait_sr1(I2C_SR1_SB) == 0U) {
        return 0U;
    }

    I2C1_DR = (unsigned int)addr8;
    if (i2c1_wait_sr1(I2C_SR1_ADDR) == 0U) {
        return 0U;
    }

    /* 读 SR1 再读 SR2 清 ADDR */
    sr1 = I2C1_SR1;
    (void)sr1;
    (void)I2C1_SR2;
    return 1U;
}

void I2C1_Init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPBEN;
    RCC_APB1ENR |= RCC_APB1ENR_I2C1EN;

    RCC_APB1RSTR |= RCC_APB1RSTR_I2C1RST;
    RCC_APB1RSTR &= ~RCC_APB1RSTR_I2C1RST;

    GPIOB_CRL &= ~(GPIOB_CRL_PB6_MASK | GPIOB_CRL_PB7_MASK);
    GPIOB_CRL |= GPIOB_CRL_PB6_AF_OD | GPIOB_CRL_PB7_AF_OD;

    i2c1_hw_apply();

    DMA1_ClockEnable();
    NVIC_IRQ_SetPriority(DMA1_Channel6_IRQn, I2C_DMA_NVIC_PRIO);
    NVIC_IRQ_Enable(DMA1_Channel6_IRQn);
    i2c1_dma_tx_done = 1U;
}

unsigned char I2C1_Write(unsigned char addr8, const unsigned char *buf, unsigned int len)
{
    unsigned int i;

    if ((buf == 0) && (len != 0U)) {
        return 0U;
    }

    if (i2c1_start_addr(addr8) == 0U) {
        return 0U;
    }

    for (i = 0U; i < len; i++) {
        if (i2c1_wait_sr1(I2C_SR1_TXE) == 0U) {
            return 0U;
        }
        I2C1_DR = (unsigned int)buf[i];
    }

    if (i2c1_wait_sr1(I2C_SR1_BTF) == 0U) {
        return 0U;
    }

    I2C1_CR1 |= I2C_CR1_STOP;
    return 1U;
}

unsigned char I2C1_WriteDma(unsigned char addr8, unsigned char ctrl,
                            const unsigned char *mem, unsigned int len)
{
    volatile unsigned int n;

    if ((mem == 0) || (len == 0U)) {
        return 0U;
    }

    if (i2c1_start_addr(addr8) == 0U) {
        return 0U;
    }

    if (i2c1_wait_sr1(I2C_SR1_TXE) == 0U) {
        return 0U;
    }
    I2C1_DR = (unsigned int)ctrl;
    if (i2c1_wait_sr1(I2C_SR1_BTF) == 0U) {
        return 0U;
    }

    i2c1_dma_tx_done = 0U;
    I2C1_CR2 |= I2C_CR2_DMAEN;
    DMA1_Channel_Start(DMA1_CHANNEL6,
                       I2C1_DMA_CCR_TX & ~DMA_CCR_HTIE,
                       I2C1_BASE + 0x10U,
                       (unsigned int)mem,
                       len);

    n = I2C_WAIT_LOOPS;
    while ((i2c1_dma_tx_done == 0U) && (n != 0U)) {
        n--;
    }
    if (i2c1_dma_tx_done == 0U) {
        I2C1_CR2 &= ~I2C_CR2_DMAEN;
        DMA1_Channel_Stop(DMA1_CHANNEL6);
        I2C1_CR1 |= I2C_CR1_STOP;
        i2c1_dma_tx_done = 1U;
        return 0U;
    }
    return 1U;
}

unsigned char I2C1_Probe(unsigned char addr8)
{
    if (i2c1_start_addr(addr8) == 0U) {
        return 0U;
    }

    I2C1_CR1 |= I2C_CR1_STOP;
    return 1U;
}

void DMA1_Channel6_IRQHandler(void)
{
    volatile unsigned int n;

    DMA1_Channel_Stop(DMA1_CHANNEL6);
    DMA1_Channel_ClearFlags(DMA1_CHANNEL6);
    I2C1_CR2 &= ~I2C_CR2_DMAEN;

    n = I2C_WAIT_LOOPS;
    while (n != 0U) {
        if ((I2C1_SR1 & I2C_SR1_BTF) != 0U) {
            break;
        }
        n--;
    }

    I2C1_CR1 |= I2C_CR1_STOP;
    i2c1_dma_tx_done = 1U;
}
