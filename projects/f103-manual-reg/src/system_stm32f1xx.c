/**
 * @file    system_stm32f1xx.c
 * @brief   系统时钟初始化（SystemInit）
 *
 * @target  STM32F103C8T6
 * @ref     核心板测试程序(PC13闪烁)/USER/system_stm32f10x.c — SetSysClockTo72
 *
 * 调用时机（勿在 main.c 中再次调用）：
 *   startup_stm32f103xb.s::Reset_Handler
 *     → 设 SP / 拷贝 .data / 清 .bss
 *     → bl SystemInit          ← 本文件
 *     → bl main
 *
 * 本函数职责：
 *   1. 将 RCC 恢复为接近复位默认态（开 HSI、清 CFGR 分频/PLL 配置等）
 *   2. SetSysClockTo72()：HSE 8 MHz × PLL×9 → SYSCLK 72 MHz
 *      APB2 = HCLK = 72 MHz（USART1 / SPI1 时钟源）
 *      APB1 = HCLK/2 = 36 MHz
 *   3. HSE 启动超时则直接 return，保持 HSI 8 MHz；main 仍会执行
 *
 * 下游依赖（HSE 成功时）：
 *   usart.c 按 PCLK2=72 MHz 写 BRR → 1500000 bps
 *   spi.c  按 PCLK2=72 MHz、DIV16 → SCK ≈ 4.5 MHz
 *
 * @see     doc/projects/f103-manual-reg.md § 启动与时钟
 * @see     doc/learn/stm32-bare-metal-bootstrap.md Q11
 * @see     doc/reference/stm32f103/md/topics/rcc-clock-hse-pll.md
 */

#include "system_stm32f1xx.h"

/* -------------------------------------------------------------------------- */
/* 寄存器基地址                                                               */
/* -------------------------------------------------------------------------- */

#define RCC_BASE   0x40021000U
#define FLASH_BASE 0x40022000U

#define RCC_CR     (*(volatile unsigned int *)(RCC_BASE + 0x00U))
#define RCC_CFGR   (*(volatile unsigned int *)(RCC_BASE + 0x04U))
#define FLASH_ACR  (*(volatile unsigned int *)(FLASH_BASE + 0x00U))

/* RCC_CR 位 */
#define RCC_CR_HSION   (1U << 0)   /**< 内部 8 MHz RC 振荡器使能 */
#define RCC_CR_HSEON   (1U << 16)  /**< 外部 HSE 振荡器使能 */
#define RCC_CR_HSERDY  (1U << 17)  /**< HSE 就绪标志 */
#define RCC_CR_PLLON   (1U << 24)  /**< PLL 使能 */
#define RCC_CR_PLLRDY  (1U << 25)  /**< PLL 就绪标志 */
#define RCC_CR_HSEBYP  (1U << 18)  /**< HSE 旁路（初始化时清除） */

/* RCC_CFGR 位域 */
#define RCC_CFGR_SW_PLL    (2U << 0)   /**< 系统时钟源选择 PLL */
#define RCC_CFGR_SWS_PLL   (2U << 2)   /**< 当前时钟源为 PLL（读回） */
#define RCC_CFGR_PPRE1_DIV2 (4U << 8)  /**< APB1 = HCLK/2（≤36 MHz） */
#define RCC_CFGR_PLLSRC_HSE (1U << 16) /**< PLL 时钟源 HSE */
#define RCC_CFGR_PLLMUL9   (7U << 18)  /**< PLL 倍频 ×9 */

/* FLASH_ACR 位 */
#define FLASH_ACR_PRFTBE   (1U << 4)   /**< 预取缓冲使能 */
#define FLASH_ACR_LATENCY2 (2U << 0)   /**< 2 等待周期（64 MHz < SYSCLK ≤ 72 MHz） */

/** @brief HSE 启动最大轮询次数（与 CMSIS HSE_STARTUP_TIMEOUT 一致） */
#define HSE_STARTUP_TIMEOUT 0x0500U

/**
 * @brief  将系统时钟配置为 72 MHz（HSE × 9）
 *
 * 配置步骤概要：
 *   1. 等待 HSE 就绪（带超时）
 *   2. Flash 等待周期设为 2，开启预取
 *   3. AHB=SYSCLK，APB2=HCLK，APB1=HCLK/2
 *   4. PLL 源 HSE，倍频 ×9，使能并等待就绪
 *   5. 切换 SYSCLK 至 PLL
 *
 * @note   若板上无 8 MHz 晶振或 HSE 失败，直接返回，继续使用 HSI 8 MHz
 */
static void SetSysClockTo72(void)
{
    unsigned int startup_counter = 0;

    RCC_CR |= RCC_CR_HSEON;
    while (!(RCC_CR & RCC_CR_HSERDY) && startup_counter < HSE_STARTUP_TIMEOUT) {
        startup_counter++;
    }

    if (!(RCC_CR & RCC_CR_HSERDY)) {
        return;
    }

    FLASH_ACR |= FLASH_ACR_PRFTBE;
    FLASH_ACR = (FLASH_ACR & ~0x7U) | FLASH_ACR_LATENCY2;

    /* 清除 HPRE / PPRE1 / PPRE2，APB1 设为 2 分频 */
    RCC_CFGR &= ~((0xFU << 4) | (7U << 8) | (7U << 11));
    RCC_CFGR |= RCC_CFGR_PPRE1_DIV2;

    /* PLL：HSE × 9 */
    RCC_CFGR &= ~((1U << 16) | (1U << 17) | (0xFU << 18));
    RCC_CFGR |= RCC_CFGR_PLLSRC_HSE | RCC_CFGR_PLLMUL9;

    RCC_CR |= RCC_CR_PLLON;
    while (!(RCC_CR & RCC_CR_PLLRDY)) {
    }

    RCC_CFGR &= ~3U;
    RCC_CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC_CFGR & 0xCU) != RCC_CFGR_SWS_PLL) {
    }
}

/**
 * @brief  复位后时钟与 RCC 默认化，并尝试升频至 72 MHz
 *
 * 唯一调用点：startup_stm32f103xb.s 中 Reset_Handler 的 `bl SystemInit`
 *（在拷贝 .data、清零 .bss 之后，进入 main 之前）。
 *
 * 先将 RCC 恢复至数据手册规定的复位状态，再调用 SetSysClockTo72。
 * 与 ST CMSIS SystemInit 前半段行为一致。
 */
void SystemInit(void)
{
    RCC_CR |= RCC_CR_HSION;
    RCC_CFGR &= 0xF8FF0000U;
    RCC_CR &= 0xFEF6FFFFU;
    RCC_CR &= ~RCC_CR_HSEBYP;
    RCC_CFGR &= 0xFF80FFFFU;

    SetSysClockTo72();
}
