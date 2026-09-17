/**
 * @file    systick.h
 * @brief   SysTick 1 ms 时基（处理器时钟，72 MHz 下 1 kHz）
 *
 * 覆盖 startup 中 weak 的 SysTick_Handler。HSE 失败则节拍变慢（与 USART 同类）。
 *
 * @see     systick.c
 */

#ifndef SYSTICK_H
#define SYSTICK_H

void SysTick_Init(void);

/** 上电后累计毫秒（约 49 天回绕） */
unsigned int SysTick_GetMs(void);

void SysTick_SetMs(unsigned int ms);

#endif /* SYSTICK_H */
