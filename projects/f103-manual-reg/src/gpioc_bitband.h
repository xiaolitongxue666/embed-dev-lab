/**
 * @file    gpioc_bitband.h
 * @brief   GPIOC ODR 位带别名访问（寄存器直写）
 *
 * @ref     核心板测试程序(PC13闪烁)/USER/GPIOLIKE51.h
 * @note    Cortex-M3 支持位带：对别名区单 bit 读写等效于原寄存器原子位操作。
 *          本头文件仅封装 GPIOC 的 ODR 输出位带；PCout 仍是对 GPIOC_ODR（0x4001100C）的 MMIO。
 *          详见 doc/learn/stm32f103-mmio-basics.md
 */

#ifndef GPIOC_BITBAND_H
#define GPIOC_BITBAND_H

/* -------------------------------------------------------------------------- */
/* 位带地址计算（参见 ARM Cortex-M3 技术参考手册 — 位带章节）                 */
/* -------------------------------------------------------------------------- */

/**
 * @brief  计算某外设寄存器某 bit 在位带别名区的地址
 * @param  addr    外设寄存器字节地址
 * @param  bitnum  bit 序号（0–31）
 */
#define BITBAND(addr, bitnum) \
    (((addr) & 0xF0000000U) + 0x02000000U + (((addr) & 0xFFFFFU) << 5) + ((bitnum) << 2))

/** @brief 将地址转为 volatile 指针并解引用 */
#define MEM_ADDR(addr) (*((volatile unsigned long *)(addr)))

/** @brief 位带别名区上的可读写 bit 对象 */
#define BIT_ADDR(addr, bitnum) MEM_ADDR(BITBAND((addr), (bitnum)))

/* -------------------------------------------------------------------------- */
/* GPIOC 输出                                                                   */
/* -------------------------------------------------------------------------- */

#define GPIOC_BASE 0x40011000U
/** @brief GPIOC ODR 寄存器偏移 +12 → 0x4001100C */
#define GPIOC_ODR_ADDR (GPIOC_BASE + 12U)

/**
 * @brief  GPIOC 端口第 n 脚输出（位带）
 * @param  n  引脚号 0–15，本工程 LED 使用 PCout(13)
 */
#define PCout(n) BIT_ADDR(GPIOC_ODR_ADDR, (n))

#endif /* GPIOC_BITBAND_H */
