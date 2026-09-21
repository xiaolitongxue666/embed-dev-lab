/**
 * @file    tim.c
 * @brief   TIM2 CH2 PWM（PA1），ARR=999
 */

#include "tim.h"

TIM_HandleTypeDef htim2;

void MX_TIM2_Init(void)
{
    TIM_OC_InitTypeDef oc = {0};

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 2;
    htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim2.Init.Period = FAN_DUTY_MAX;
    htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.RepetitionCounter = 0;
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK) {
        Error_Handler();
    }

    oc.OCMode = TIM_OCMODE_PWM1;
    oc.Pulse = 0;
    oc.OCPolarity = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &oc, TIM_CHANNEL_2) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2) != HAL_OK) {
        Error_Handler();
    }
}

void Fan_SetDuty(uint32_t duty)
{
    if (duty > FAN_DUTY_MAX) {
        duty = FAN_DUTY_MAX;
    }
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, duty);
}
