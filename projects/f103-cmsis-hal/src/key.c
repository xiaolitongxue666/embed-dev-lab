/**
 * @file    key.c
 * @brief   PB13 EXTI 双沿，主循环 Key_TakeEdge 取走标志
 */

#include "gpio.h"
#include "key.h"

static volatile uint8_t s_key_edge;

void Key_ExtiInit(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Pin = KEY_PIN;
    gpio.Mode = GPIO_MODE_IT_RISING_FALLING;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(KEY_PORT, &gpio);

    HAL_NVIC_SetPriority(EXTI15_10_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}

uint8_t Key_TakeEdge(void)
{
    uint8_t edge;

    edge = s_key_edge;
    s_key_edge = 0U;
    return edge;
}

void HAL_GPIO_EXTI_Callback(uint16_t gpio_pin)
{
    if (gpio_pin == KEY_PIN) {
        s_key_edge = 1U;
    }
}
