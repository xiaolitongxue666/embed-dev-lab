/**
 * @file    stm32f1xx_hal_conf.h
 * @brief   HAL 模块使能（PC13 闪烁最小集）
 */

#ifndef STM32F1XX_HAL_CONF_H
#define STM32F1XX_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx.h"

#define HAL_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED

#define HSE_VALUE    8000000U
#define HSE_STARTUP_TIMEOUT 0x0500U
#define HSI_VALUE    8000000U
#define LSI_VALUE    40000U
#define LSE_VALUE    32768U
#define LSE_STARTUP_TIMEOUT 5000U

#define VDD_VALUE                    3300U
#define TICK_INT_PRIORITY            0x0FU
#define USE_RTOS                     0U
#define PREFETCH_ENABLE              1U

#include "stm32f1xx_hal_rcc.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_pwr.h"
#include "stm32f1xx_hal_flash.h"
#include "stm32f1xx_hal_cortex.h"

#ifdef __cplusplus
}
#endif

/* Exported macro ------------------------------------------------------------*/
#ifdef USE_FULL_ASSERT
#define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
void assert_failed(uint8_t *file, uint32_t line);
#else
#define assert_param(expr) ((void)0U)
#endif

#endif /* STM32F1XX_HAL_CONF_H */
