/**
 * @file    i2c.c
 * @brief   I2C1 纯寄存器主机写：400 kHz Fast、duty 2:1、阻塞 + 超时
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
 * OAR1 bit14 手册要求软件保持为 1。
 *
 * 时钟前提（system_stm32f1xx.c）：
 *   SYSCLK 72 MHz，APB1 /2 → PCLK1 = 36 MHz（I2C1 时钟源）
 *   CR2.FREQ=36；Fast 400 kHz duty 2：CCR=30、FS=1；TRISE=11
 *
 * @see     doc/hardware/stm32f103-peripherals.md
 * @see     doc/reference/stm32f103/md/topics/i2c1-master-polling.md
 * @see     RM0008 I2C 章
 */

#include "i2c.h"

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

    I2C1_CR1 &= ~I2C_CR1_PE;
    I2C1_CR2 = I2C_CR2_FREQ;
    I2C1_CCR = I2C_CCR_FS | I2C_CCR_400KHZ;
    I2C1_TRISE = I2C_TRISE_FAST;
    I2C1_OAR1 = I2C_OAR1_BIT14;
    I2C1_CR1 = I2C_CR1_ACK | I2C_CR1_PE;
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

unsigned char I2C1_Probe(unsigned char addr8)
{
    if (i2c1_start_addr(addr8) == 0U) {
        return 0U;
    }

    I2C1_CR1 |= I2C_CR1_STOP;
    return 1U;
}
