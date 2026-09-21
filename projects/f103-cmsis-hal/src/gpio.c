/**
 * @file    gpio.c
 * @brief   MX_GPIO_Init：Backup 域 PC13、SPI 双片选空闲高
 */

#include "gpio.h"

/**
 * @brief  初始化 PC13 推挽、PA3/PA8 片选
 *
 * PC13 须先于 GPIO 配置：
 *   1. __HAL_RCC_PWR_CLK_ENABLE()
 *   2. HAL_PWR_EnableBkUpAccess()
 *   3. __HAL_RCC_GPIOC_CLK_ENABLE()
 *   4. HAL_GPIO_Init
 */
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Pin = BOARD_LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(BOARD_LED_PORT, &gpio);

    gpio.Pin = BMP280_CS_PIN | LSM6DS3_CS_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
    HAL_GPIO_WritePin(BMP280_CS_PORT, BMP280_CS_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(LSM6DS3_CS_PORT, LSM6DS3_CS_PIN, GPIO_PIN_SET);

    gpio.Pin = EXT_LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(EXT_LED_PORT, &gpio);
}
