/**
 * @file    tim2.h
 * @brief   TIM2_CH2（PA1）PWM：JY003 风扇占空比
 *
 * TIM2 默认 remap（CH2=PA1）。APB1 预分频≠1 时 TIMxCLK=2×PCLK1=72 MHz
 * （RM0008 RCC 时钟树）。PSC=2、ARR=999 → 24 kHz，占空比 0..999。
 * 计划稿 PSC=35+ARR=999 实为 2 kHz；24 kHz 避开听感啸叫并保留 1000 档。
 *
 * @see     tim2.c
 * @see     doc/hardware/stm32f103-peripherals.md
 * @see     RM0008 通用定时器 PWM mode 1；TIM2 基址 0x40000000
 */

#ifndef TIM2_H
#define TIM2_H

/** ARR=999：CCR2 有效范围 0..999 */
#define FAN_DUTY_MAX 999U
/** 旋钮 raw 低于此值时占空比为 0，避免停不转 */
#define FAN_KNOB_DEADZONE 80U

/**
 * @brief  开启 TIM2 时钟、PA1 复用推挽、PWM1、CEN
 */
void TIM2_PWM_Init(void);

/**
 * @brief  写 TIM2_CCR2；duty>999 时钳到 999
 */
void Fan_SetDuty(unsigned int duty);

#endif /* TIM2_H */
