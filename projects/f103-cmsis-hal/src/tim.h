/**
 * @file    tim.h
 * @brief   TIM2_CH2（PA1）PWM：JY003 风扇占空比
 *
 * PSC=2、ARR=999 → 24 kHz（APB1 倍频后 TIMCLK=72 MHz）。
 */

#ifndef TIM_H
#define TIM_H

#include "main.h"

#define FAN_DUTY_MAX        999U
#define FAN_KNOB_DEADZONE   80U

void MX_TIM2_Init(void);
void Fan_SetDuty(uint32_t duty);

#endif /* TIM_H */
