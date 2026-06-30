/**
 * @file    usart.h
 * @brief   USART1（PA9 TX / PA10 RX）纯寄存器调试口
 *
 * @see     usart.c — 寄存器初始化与 USART1_Write
 * @see     doc/projects/f103-manual-reg.md — § USART1 与硬件接线
 */

#ifndef USART_H
#define USART_H

void USART1_Init(void);
void USART1_Write(const char *buf, int len);

#endif /* USART_H */
