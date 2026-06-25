/**
 * @file    stm32f1xx_hal_msp.c
 * @brief   HAL MSP 回调（本 demo 无额外外设 MSP 初始化）
 */

#include "main.h"

void HAL_MspInit(void)
{
}

void HAL_GPIO_MspInit(GPIO_InitTypeDef *gpio_init_struct)
{
    (void)gpio_init_struct;
}
