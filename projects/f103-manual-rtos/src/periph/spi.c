/**
 * @file    spi.c
 * @brief   SPI1 主机：PA5/6/7，PA3=BMP280 Mode0，PA8=LSM6 Mode3；DMA1 CH2 RX / CH3 TX
 *
 * 默认映射；Init 默认 Mode 3，两 CS 空闲高。多字节走 DMA1_Channel_WaitTc（无 IRQ）。
 * 软/硬 SPI 对照见项目文档。BR=DIV16 → SCK ≈ 4.5 MHz（PCLK2=72 MHz）。
 *
 * @see     spi.h
 * @see     doc/projects/f103-manual-reg.md
 * @see     doc/hardware/stm32f103-peripherals.md
 */

#include "dma.h"
#include "spi.h"

/* -------------------------------------------------------------------------- */
/* 外设基地址与寄存器（RM0008）                                                 */
/* -------------------------------------------------------------------------- */

#define RCC_BASE      0x40021000U
#define RCC_APB2ENR   (*(volatile unsigned int *)(RCC_BASE + 0x18U))

#define GPIOA_BASE    0x40010800U
#define GPIOA_CRL     (*(volatile unsigned int *)(GPIOA_BASE + 0x00U))
#define GPIOA_CRH     (*(volatile unsigned int *)(GPIOA_BASE + 0x04U))
#define GPIOA_BSRR    (*(volatile unsigned int *)(GPIOA_BASE + 0x10U))
#define GPIOA_BRR     (*(volatile unsigned int *)(GPIOA_BASE + 0x14U))

/** @brief SPI1 基地址（APB2）；以下为硬件 SPI 控制器寄存器，非软 SPI 的 GPIO 别名 */
#define SPI1_BASE     0x40013000U
/** @brief CR1：Control Register 1（配置主从、Mode、分频、SPE） */
#define SPI1_CR1      (*(volatile unsigned int *)(SPI1_BASE + 0x00U))
#define SPI1_CR2      (*(volatile unsigned int *)(SPI1_BASE + 0x04U))
/** @brief SR：Status Register（TXE / RXNE） */
#define SPI1_SR       (*(volatile unsigned int *)(SPI1_BASE + 0x08U))
/** @brief DR：Data Register（写→硬件经 MOSI 发出；读←硬件经 MISO 收到） */
#define SPI1_DR       (*(volatile unsigned int *)(SPI1_BASE + 0x0CU))

/* RCC_APB2ENR */
#define RCC_APB2ENR_IOPAEN  (1U << 2)   /**< GPIOA */
#define RCC_APB2ENR_SPI1EN  (1U << 12)  /**< SPI1 */

/* GPIOA_CRL：PA3 / PA5–PA7 各占 4 bit */
#define GPIOA_CRL_PA3_MASK  (0xFU << 12)
#define GPIOA_CRL_PA5_MASK  (0xFU << 20)
#define GPIOA_CRL_PA6_MASK  (0xFU << 24)
#define GPIOA_CRL_PA7_MASK  (0xFU << 28)

/** PA3 推挽输出 50 MHz：CNF=00 MODE=11 → 0x3 */
#define GPIOA_CRL_PA3_OUT_PP (0x3U << 12)
/** PA5/PA7 复用推挽 50 MHz：CNF=10 MODE=11 → 0xB */
#define GPIOA_CRL_PA5_AF_PP  (0xBU << 20)
#define GPIOA_CRL_PA7_AF_PP  (0xBU << 28)
/** PA6 浮空输入（高阻）：CNF=01 MODE=00 → 0x4 */
#define GPIOA_CRL_PA6_IN_FLOAT (0x4U << 24)
/** PA8 在 CRH[3:0]：推挽输出 50 MHz，CNF=00 MODE=11 → 0x3 */
#define GPIOA_CRH_PA8_MASK     (0xFU << 0)
#define GPIOA_CRH_PA8_OUT_PP   (0x3U << 0)

/* SPI_CR1 位 */
#define SPI_CR1_CPHA      (1U << 0)
#define SPI_CR1_CPOL      (1U << 1)
#define SPI_CR1_MSTR      (1U << 2)
#define SPI_CR1_BR_DIV16  (3U << 3)   /**< BR[2:0]=011 → fPCLK/16 */
#define SPI_CR1_SPE       (1U << 6)
#define SPI_CR1_SSI       (1U << 8)
#define SPI_CR1_SSM       (1U << 9)

#define SPI_CR2_RXDMAEN   (1U << 0)
#define SPI_CR2_TXDMAEN   (1U << 1)

/* SPI_SR 位 */
#define SPI_SR_RXNE       (1U << 0)
#define SPI_SR_TXE        (1U << 1)
#define SPI_SR_BSY        (1U << 7)

#define SPI1_DMA_MAX      32U
#define SPI1_DMA_GUARD    100000U
#define SPI1_DMA_CCR_RX   (DMA_CCR_MINC)
#define SPI1_DMA_CCR_TX   (DMA_CCR_DIR | DMA_CCR_MINC)

static unsigned char spi1_dma_tx[SPI1_DMA_MAX];
static unsigned char spi1_dma_rx[SPI1_DMA_MAX];

/** BMP280 CSB = PA3；LSM6 CS = PA8。低有效。 */
#define BMP280_CS_PIN     3U
#define LSM6DS3_CS_PIN    8U

/**
 * @brief  初始化 SPI1 主机、PA3/PA8 片选
 *
 * 顺序：开时钟 → 配 GPIO → 两 CS 拉高 → 写 CR1（默认 Mode 3）→ 置 SPE。
 */
void SPI1_Init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPAEN | RCC_APB2ENR_SPI1EN;
    DMA1_ClockEnable();

    GPIOA_CRL &= ~(GPIOA_CRL_PA3_MASK | GPIOA_CRL_PA5_MASK |
                   GPIOA_CRL_PA6_MASK | GPIOA_CRL_PA7_MASK);
    GPIOA_CRL |= GPIOA_CRL_PA3_OUT_PP | GPIOA_CRL_PA5_AF_PP |
                 GPIOA_CRL_PA6_IN_FLOAT | GPIOA_CRL_PA7_AF_PP;

    GPIOA_CRH = (GPIOA_CRH & ~GPIOA_CRH_PA8_MASK) | GPIOA_CRH_PA8_OUT_PP;

    /* PA3 空闲高；PA8 同步拉高，避免浮空当 CS */
    GPIOA_BSRR = (1U << BMP280_CS_PIN) | (1U << LSM6DS3_CS_PIN);

    /*
     * Master | SSM | SSI | CPOL | CPHA | BR=DIV16 | SPE
     * SSM+SSI：软件管理 NSS，内部 NSS 视为高，避免 Mode Fault。
     * 默认 Mode 3，兼容尚未改 Mode 的 LSM6 调用。
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
    unsigned int guard;

    guard = 100000U;
    while ((SPI1_SR & SPI_SR_TXE) == 0U) {
        guard--;
        if (guard == 0U) {
            return 0xFFU;
        }
    }
    SPI1_DR = (unsigned int)tx;

    guard = 100000U;
    while ((SPI1_SR & SPI_SR_RXNE) == 0U) {
        guard--;
        if (guard == 0U) {
            return 0xFFU;
        }
    }
    return (unsigned char)(SPI1_DR & 0xFFU);
}

unsigned char SPI1_TransferBytes(const unsigned char *tx, unsigned char *rx,
                                 unsigned int len)
{
    unsigned int i;
    unsigned int guard;

    if ((len == 0U) || (len > SPI1_DMA_MAX) || (tx == 0) || (rx == 0)) {
        return 0U;
    }

    for (i = 0U; i < len; i++) {
        spi1_dma_tx[i] = tx[i];
    }

    if ((SPI1_SR & SPI_SR_RXNE) != 0U) {
        (void)SPI1_DR;
    }

    SPI1_CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN;
    DMA1_Channel_Start(DMA1_CHANNEL2,
                       SPI1_DMA_CCR_RX,
                       SPI1_BASE + 0x0CU,
                       (unsigned int)spi1_dma_rx,
                       len);
    DMA1_Channel_Start(DMA1_CHANNEL3,
                       SPI1_DMA_CCR_TX,
                       SPI1_BASE + 0x0CU,
                       (unsigned int)spi1_dma_tx,
                       len);

    if (DMA1_Channel_WaitTc(DMA1_CHANNEL2, SPI1_DMA_GUARD) == 0U) {
        DMA1_Channel_Stop(DMA1_CHANNEL2);
        DMA1_Channel_Stop(DMA1_CHANNEL3);
        SPI1_CR2 &= ~(SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);
        return 0U;
    }
    (void)DMA1_Channel_WaitTc(DMA1_CHANNEL3, SPI1_DMA_GUARD);

    DMA1_Channel_Stop(DMA1_CHANNEL2);
    DMA1_Channel_Stop(DMA1_CHANNEL3);
    SPI1_CR2 &= ~(SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN);

    guard = SPI1_DMA_GUARD;
    while ((SPI1_SR & SPI_SR_BSY) != 0U) {
        guard--;
        if (guard == 0U) {
            break;
        }
    }

    for (i = 0U; i < len; i++) {
        rx[i] = spi1_dma_rx[i];
    }
    return 1U;
}

void SPI1_SetMode0(void)
{
    SPI1_CR1 &= ~SPI_CR1_SPE;
    SPI1_CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);
    SPI1_CR1 |= SPI_CR1_SPE;
}

void SPI1_SetMode3(void)
{
    SPI1_CR1 &= ~SPI_CR1_SPE;
    SPI1_CR1 |= SPI_CR1_CPOL | SPI_CR1_CPHA;
    SPI1_CR1 |= SPI_CR1_SPE;
}

void BMP280_CsLow(void)
{
    GPIOA_BRR = (1U << BMP280_CS_PIN);
}

void BMP280_CsHigh(void)
{
    GPIOA_BSRR = (1U << BMP280_CS_PIN);
}

void LSM6DS3_CsLow(void)
{
    GPIOA_BRR = (1U << LSM6DS3_CS_PIN);
}

void LSM6DS3_CsHigh(void)
{
    GPIOA_BSRR = (1U << LSM6DS3_CS_PIN);
}
