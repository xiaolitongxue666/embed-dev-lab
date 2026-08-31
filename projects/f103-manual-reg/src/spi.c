/**
 * @file    spi.c
 * @brief   SPI1 纯寄存器主机：Mode 3、软件 NSS（PA4）、阻塞收发
 *
 * @target  STM32F103C8T6，SPI1 默认映射 PA5(SCK)/PA6(MISO)/PA7(MOSI)，PA4 作 GPIO CS
 *
 * 硬件接线（LSM6DS3 / LSM6DS3TR 模块丝印 → MCU）：
 *   模块 3V3 ← 核心板 3.3 V（勿接 VIN）
 *   模块 GND ← 核心板 GND（共地）
 *   模块 SCL ← PA5（SPI1_SCK）
 *   模块 SDA ← PA7（SPI1_MOSI / SDI）
 *   模块 SAO → PA6（SPI1_MISO / SDO）
 *   模块 CS  ← PA4（GPIO 软件片选，低有效）
 *   INT1 / INT2 / OCS / SCX / SDX 本 demo 不接
 *
 * 时钟前提（system_stm32f1xx.c）：
 *   SYSCLK 72 MHz，APB2 不分频 → PCLK2 = 72 MHz（SPI1 时钟源）
 *   BR = DIV16 → SCK ≈ 4.5 MHz（低于 LSM6DS3 10 MHz 上限）
 *
 * Mode 3：CPOL=1（空闲高）、CPHA=1（第二边沿采样），对齐 LSM6DS3 手册时序图。
 *
 * @see     doc/hardware/stm32f103-peripherals.md
 * @see     doc/reference/lsm6ds3/md/topics/electrical-spi-timing.md
 * @see     RM0008 SPI 章（SPI1 基址 0x40013000）
 */

#include "spi.h"

/* -------------------------------------------------------------------------- */
/* 外设基地址与寄存器（RM0008）                                                 */
/* -------------------------------------------------------------------------- */

#define RCC_BASE      0x40021000U
#define RCC_APB2ENR   (*(volatile unsigned int *)(RCC_BASE + 0x18U))

#define GPIOA_BASE    0x40010800U
#define GPIOA_CRL     (*(volatile unsigned int *)(GPIOA_BASE + 0x00U))
#define GPIOA_BSRR    (*(volatile unsigned int *)(GPIOA_BASE + 0x10U))
#define GPIOA_BRR     (*(volatile unsigned int *)(GPIOA_BASE + 0x14U))

/** @brief SPI1 基地址（APB2） */
#define SPI1_BASE     0x40013000U
#define SPI1_CR1      (*(volatile unsigned int *)(SPI1_BASE + 0x00U))
#define SPI1_SR       (*(volatile unsigned int *)(SPI1_BASE + 0x08U))
#define SPI1_DR       (*(volatile unsigned int *)(SPI1_BASE + 0x0CU))

/* RCC_APB2ENR */
#define RCC_APB2ENR_IOPAEN  (1U << 2)   /**< GPIOA */
#define RCC_APB2ENR_SPI1EN  (1U << 12)  /**< SPI1 */

/* GPIOA_CRL：PA4–PA7 各占 4 bit */
#define GPIOA_CRL_PA4_MASK  (0xFU << 16)
#define GPIOA_CRL_PA5_MASK  (0xFU << 20)
#define GPIOA_CRL_PA6_MASK  (0xFU << 24)
#define GPIOA_CRL_PA7_MASK  (0xFU << 28)

/** PA4 推挽输出 50 MHz：CNF=00 MODE=11 → 0x3 */
#define GPIOA_CRL_PA4_OUT_PP (0x3U << 16)
/** PA5/PA7 复用推挽 50 MHz：CNF=10 MODE=11 → 0xB */
#define GPIOA_CRL_PA5_AF_PP  (0xBU << 20)
#define GPIOA_CRL_PA7_AF_PP  (0xBU << 28)
/** PA6 浮空输入：CNF=01 MODE=00 → 0x4 */
#define GPIOA_CRL_PA6_IN_FLOAT (0x4U << 24)

/* SPI_CR1 位 */
#define SPI_CR1_CPHA      (1U << 0)
#define SPI_CR1_CPOL      (1U << 1)
#define SPI_CR1_MSTR      (1U << 2)
#define SPI_CR1_BR_DIV16  (3U << 3)   /**< BR[2:0]=011 → fPCLK/16 */
#define SPI_CR1_SPE       (1U << 6)
#define SPI_CR1_SSI       (1U << 8)
#define SPI_CR1_SSM       (1U << 9)

/* SPI_SR 位 */
#define SPI_SR_RXNE       (1U << 0)
#define SPI_SR_TXE        (1U << 1)

/** PA4 片选：低有效 */
#define SPI1_CS_PIN       4U

/**
 * @brief  初始化 SPI1 主机与 PA4 CS
 *
 * 顺序：开时钟 → 配 GPIO → 写 CR1（含 Mode 3、软件 NSS、分频）→ 置 SPE。
 * CS 默认拉高（空闲）。
 */
void SPI1_Init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN;

    GPIOA_CRL &= ~(GPIOA_CRL_PA4_MASK | GPIOA_CRL_PA5_MASK |
                   GPIOA_CRL_PA6_MASK | GPIOA_CRL_PA7_MASK);
    GPIOA_CRL |= GPIOA_CRL_PA4_OUT_PP | GPIOA_CRL_PA5_AF_PP |
                 GPIOA_CRL_PA6_IN_FLOAT | GPIOA_CRL_PA7_AF_PP;

    /* CS 空闲高 */
    GPIOA_BSRR = (1U << SPI1_CS_PIN);

    /*
     * Master | SSM | SSI | CPOL | CPHA | BR=DIV16 | SPE
     * SSM+SSI：软件管理 NSS，内部 NSS 视为高，避免 Mode Fault。
     */
    SPI1_CR1 = SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI |
               SPI_CR1_CPOL | SPI_CR1_CPHA | SPI_CR1_BR_DIV16 |
               SPI_CR1_SPE;
}

/**
 * @brief  全双工交换 1 字节（阻塞）
 * @param  tx  主机发出的字节
 * @return 从机在同一时钟节拍回送的字节
 */
unsigned char SPI1_TransferByte(unsigned char tx)
{
    while ((SPI1_SR & SPI_SR_TXE) == 0U) {
        /* 等发送缓冲空 */
    }
    SPI1_DR = (unsigned int)tx;

    while ((SPI1_SR & SPI_SR_RXNE) == 0U) {
        /* 等接收缓冲非空 */
    }
    return (unsigned char)(SPI1_DR & 0xFFU);
}

void SPI1_CsLow(void)
{
    GPIOA_BRR = (1U << SPI1_CS_PIN);
}

void SPI1_CsHigh(void)
{
    GPIOA_BSRR = (1U << SPI1_CS_PIN);
}
