/**
 * @file    stm32f1xx_hal_msp.c
 * @brief   HAL MSP（MCU Support Package）回调
 *
 * @note    CubeMX 工程中在此实现外设底层初始化（时钟、GPIO 复用、DMA 等）。
 *          本 demo GPIO/RCC 均在 main.c 内完成，MSP 回调保持空实现。
 *
 * 调用关系（HAL 库内部）：
 *   HAL_Init()        → HAL_MspInit()
 *   HAL_GPIO_Init()   → HAL_GPIO_MspInit()（若需单独使能端口时钟可在此实现）
 *
 * 与 f103-manual-reg 对照：manual-reg 无 MSP 层，寄存器操作集中在 main.c。
 */

#include "main.h"

/** @brief  全局 MSP：HAL 库初始化时调用一次 */
void HAL_MspInit(void)
{
}

/**
 * @brief  单引脚 GPIO MSP；本 demo 时钟已在 MX_GPIO_Init 中使能
 * @param  gpio_init_struct  HAL 传入的引脚配置（本实现未使用）
 */
void HAL_GPIO_MspInit(GPIO_InitTypeDef *gpio_init_struct)
{
    (void)gpio_init_struct;
}
