/**
 * @file    stm32f1xx_hal_msp.c
 * @brief   HAL MSP（MCU Support Package）回调
 *
 * @note    CubeMX 工程中在此实现外设底层初始化（时钟、GPIO 复用、DMA 等）。
 *          PC13 GPIO 在 main.c；USART1 引脚与时钟在本文件 HAL_UART_MspInit。
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

/**
 * @brief  USART1 MSP：PA9 TX（复用推挽）、PA10 RX（浮空输入）
 * @note   F103 默认映射，无需 AFIO USART1 重映射
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio = {0};

    if (huart->Instance != USART1) {
        return;
    }

    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1) {
        return;
    }

    __HAL_RCC_USART1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);
}
