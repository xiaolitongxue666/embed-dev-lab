/**
 * @file    stm32f1xx_hal_conf.h
 * @brief   HAL 模块裁剪与全局参数（PC13 闪烁最小集）
 *
 * @note    本文件由工程维护；third_party/hal/Inc 为 ST 原版头文件。
 *          仅使能本 demo 用到的 HAL 模块，减小代码体积与编译时间。
 *
 * 使能模块与用途：
 *   HAL_GPIO   — PC13 输出、USART1 PA9/PA10
 *   HAL_RCC    — HSE/PLL 72 MHz 时钟
 *   HAL_PWR    — Backup 域 DBP（PC13 前置条件）
 *   HAL_FLASH  — 时钟配置时 Flash 等待周期
 *   HAL_CORTEX — HAL_Init / 内核相关
 *   HAL_UART   — USART1 调试口（HAL_UART_Transmit，无 printf）
 *
 * assert_param：未定义 USE_FULL_ASSERT 时展开为空，避免链接 assert_failed
 */

#ifndef STM32F1XX_HAL_CONF_H
#define STM32F1XX_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx.h"

/* --- 模块使能（须在 #include "stm32f1xx_hal_xxx.h" 之前） --- */
#define HAL_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

/* --- 振荡器频率（与硬件晶振一致；CMake 亦通过 HSE_VALUE=8000000U 传递） --- */
#define HSE_VALUE    8000000U
#define HSE_STARTUP_TIMEOUT 0x0500U
#define HSI_VALUE    8000000U
#define LSI_VALUE    40000U
#define LSE_VALUE    32768U
#define LSE_STARTUP_TIMEOUT 5000U

/* --- 系统参数 --- */
#define VDD_VALUE                    3300U
#define TICK_INT_PRIORITY            0x0FU
#define USE_RTOS                     0U
#define PREFETCH_ENABLE              1U

#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_pwr.h"
#include "stm32f1xx_hal_flash.h"
#include "stm32f1xx_hal_cortex.h"
#include "stm32f1xx_hal_dma.h"
#include "stm32f1xx_hal_uart.h"

#ifdef __cplusplus
}
#endif

/* HAL 参数校验宏：Release 构建为空操作，避免 third_party 中 assert_param 未定义 */
#ifdef USE_FULL_ASSERT
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif

#endif /* STM32F1XX_HAL_CONF_H */
