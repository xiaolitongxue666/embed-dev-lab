/**
 * @file    usart.h
 * @brief   USART1（PA9 TX / PA10 RX）纯寄存器调试口
 *
 * PA9/PA10：DS5319 标 FT；默认映射，勿 USART1_REMAP（抢 PB6/PB7 I2C1）。
 * 勿用 PA13/PA14（SWD）。
 *
 * @see     usart.c — 寄存器初始化与 USART1_Write
 * @see     doc/projects/f103-manual-reg.md — § USART1 与硬件接线
 * @see     doc/hardware/stm32f103c8t6-pinout.md
 */

#ifndef USART_H
#define USART_H

void USART1_Init(void);
void USART1_Write(const char *buf, int len);

#endif /* USART_H */
