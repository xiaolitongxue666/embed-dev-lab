/**
 * @file    main.c
 * @brief   STM32F103C8T6 核心板 PC13 LED 闪烁（纯寄存器实现）
 *
 * @target  STM32F103C8T6（Medium-density，64 KB Flash / 20 KB RAM）
 * @ref     install_packet/.../核心板测试程序(PC13闪烁)/USER/main.c
 *
 * @note    本模块无串口/printf；若后续添加日志，输出语言统一为英文。
 *          时钟初始化见 system_stm32f10x.c（Reset_Handler 中 SystemInit 调用）。
 */

#include "gpio_like51.h"

/* -------------------------------------------------------------------------- */
/* 外设基地址与寄存器映射（参考 RM0008）                                      */
/* -------------------------------------------------------------------------- */

/** @brief RCC 寄存器基地址 */
#define RCC_BASE     0x40021000U
/** @brief APB2 外设时钟使能（GPIOC 位于 APB2） */
#define RCC_APB2ENR  (*(volatile unsigned int *)(RCC_BASE + 0x18U))
/** @brief APB1 外设时钟使能（PWR 位于 APB1） */
#define RCC_APB1ENR  (*(volatile unsigned int *)(RCC_BASE + 0x1CU))

/** @brief PWR 电源控制寄存器基地址 */
#define PWR_BASE 0x40007000U
/** @brief PWR 控制寄存器（含 DBP 备份域写保护位） */
#define PWR_CR   (*(volatile unsigned int *)(PWR_BASE + 0x00U))

/** @brief GPIOC 基地址 */
#define GPIOC_BASE 0x40011000U
/** @brief GPIOC 端口配置高寄存器（PC8–PC15，含 PC13） */
#define GPIOC_CRH  (*(volatile unsigned int *)(GPIOC_BASE + 0x04U))

/* RCC_APB1ENR 位定义 */
#define RCC_APB1ENR_PWREN  (1U << 28)  /**< 使能 PWR 模块时钟 */

/* RCC_APB2ENR 位定义 */
#define RCC_APB2ENR_IOPCEN (1U << 4)   /**< 使能 GPIOC 端口时钟 */

/* PWR_CR 位定义 */
#define PWR_CR_DBP         (1U << 8)   /**< 关闭 Backup 域写保护 */

/* GPIOC_CRH 中 PC13 配置域：bit[23:20]，CNF=00 MODE=11 → 50 MHz 推挽输出 */
#define GPIOC_CRH_PC13_MASK   (0xFU << 20)
#define GPIOC_CRH_PC13_OUT_PP (3U << 20)

/** @brief  LED 所接 GPIO 引脚编号（核心板板载 LED → PC13） */
#define LED_PIN 13U

/**
 * @brief  软件延时（忙等待）
 * @param  count  递减计数初值，与 CPU 主频相关
 * @note   厂商例程使用 0xFFFFF 两次作为半周期；72 MHz 下肉眼可见闪烁
 */
static void delay(volatile unsigned int count)
{
    while (count != 0U) {
        count--;
    }
}

/**
 * @brief  初始化 PC13 为推挽输出
 *
 * PC13 属于 Backup 域，必须先：
 *   1. 开启 PWR 时钟（RCC_APB1ENR.PWREN）
 *   2. 置位 DBP（PWR_CR.DBP），解除 Backup 域写保护
 * 否则对 GPIOC_CRH 的写入会被硬件忽略，LED 不亮。
 *
 * GPIO 模式对齐厂商例程：Out_PP + 50 MHz（CRH[23:20] = 0b0011）。
 */
static void gpio_configuration(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_PWREN;
    PWR_CR |= PWR_CR_DBP;

    RCC_APB2ENR |= RCC_APB2ENR_IOPCEN;

    GPIOC_CRH &= ~GPIOC_CRH_PC13_MASK;
    GPIOC_CRH |= GPIOC_CRH_PC13_OUT_PP;
}

/**
 * @brief  程序入口
 * @note   多数核心板 LED 为低电平点亮：PCout(13)=0 亮，=1 灭
 */
int main(void)
{
    gpio_configuration();

    for (;;) {
        PCout(LED_PIN) = 1;
        delay(0xFFFFFU);
        delay(0xFFFFFU);

        PCout(LED_PIN) = 0;
        delay(0xFFFFFU);
        delay(0xFFFFFU);
    }
}
