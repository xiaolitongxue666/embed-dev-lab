/**
 * @file    spi.c
 * @brief   SPI1 纯寄存器主机：Mode 3、软件 NSS（PA4）、阻塞收发
 *
 * @target  STM32F103C8T6，SPI1 默认映射 PA5(SCK)/PA6(MISO)/PA7(MOSI)，PA4 作 GPIO CS
 *
 * ---------------------------------------------------------------------------
 * 实现方式：硬件 SPI（不是 GPIO 模拟）
 *
 * 常见「直接操作四根线」多半是 **软 SPI / bit-bang**：用普通 GPIO 自己
 * 翻转 SCK、一位一位写 MOSI、读 MISO、拉 CS——移位时序由 CPU 循环完成。
 *
 * 本文件用的是 **单片机自带的硬件 SPI1**：物理上仍是同一套 4 线，但移位
 * 与产生 SCK 由片内 SPI 外设完成。软件只写 CR1/SR/DR（及 GPIO 拉 CS），
 * 再由硬件去驱动 SCK/MOSI/MISO。
 *
 * | 做法 | 软件写什么 | 谁产生 SCK / 移位 |
 * |------|------------|-------------------|
 * | 软 SPI（GPIO 模拟） | 自己翻 GPIO：CS → MOSI/SCK 位操作 → 读 MISO | CPU 循环 |
 * | 硬件 SPI（本文件） | 配 CR1；写 DR 发字节；读 SR/DR 收字节；GPIO 拉 CS | 片内 SPI1 |
 *
 * 引脚映射（仍是标准 4 线）：
 *   PA5 → SCK，PA7 → MOSI，PA6 → MISO（复用到 SPI1）
 *   PA4 → CS（普通 GPIO，软件 CsLow/CsHigh，与软 SPI「先拉 CS」相同）
 *
 * ---------------------------------------------------------------------------
 * 两层命名（信号线 ≠ 寄存器）
 *
 * 【板级 / 协议层】杜邦线上的标准名：
 *   SCK（SCLK）  主机输出时钟，决定通信速率
 *   MOSI         Master Out Slave In（主机发 → 从机收）
 *   MISO         Master In Slave Out（从机发 → 主机收）
 *   CS / NSS     片选，低有效；本工程用 GPIO 软件拉
 *
 * 【MCU 外设层】RM0008 片内 SPI1 的 MMIO（与 USART 的 SR/DR 同类）：
 *   CR1  Control Register 1 — 主从、CPOL/CPHA、波特分频、SPE
 *   SR   Status Register    — TXE / RXNE
 *   DR   Data Register      — 写=经 MOSI 发出；读=经 MISO 收到
 *   （名称以手册为准，勿改成自造缩写；不是 SCK/MOSI 的别名）
 *
 * 对应关系：
 *   写 CR1 → 设定如何产生 SCK、采样沿、速率
 *   写 DR  → 硬件在 SCK 节拍下 MOSI 移出、MISO 移入
 *   读 SR  → 判断发送缓冲空 / 接收缓冲满后再读写 DR
 *   写 GPIO PA4 → 拉低/拉高 CS
 * ---------------------------------------------------------------------------
 *
 * 硬件接线（LSM6DS3 / LSM6DS3TR 模块丝印 → MCU 标准 SPI 名）：
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
 * @see     doc/projects/f103-manual-reg.md（软 SPI vs 硬件 SPI）
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

/** @brief SPI1 基地址（APB2）；以下为硬件 SPI 控制器寄存器，非软 SPI 的 GPIO 别名 */
#define SPI1_BASE     0x40013000U
/** @brief CR1：Control Register 1（配置主从、Mode、分频、SPE） */
#define SPI1_CR1      (*(volatile unsigned int *)(SPI1_BASE + 0x00U))
/** @brief SR：Status Register（TXE / RXNE） */
#define SPI1_SR       (*(volatile unsigned int *)(SPI1_BASE + 0x08U))
/** @brief DR：Data Register（写→硬件经 MOSI 发出；读←硬件经 MISO 收到） */
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
