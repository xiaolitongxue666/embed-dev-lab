/**
 * @file    usart.h
 * @brief   USART1（PA9 TX / PA10 RX）HAL 调试口
 *
 * @note    本工程用 USART1_WriteStr → HAL_UART_Transmit 发串口，**不用 printf**，
 *          故无 syscalls.c / _write（不链 libc stdout）。HAL 只管发字节；若要用
 *          printf 须自写 _write，见 f103-manual-reg 与 doc/learn/newlib-nosys-stdio-retarget.md
 *
 * @see     doc/projects/f103-cmsis-hal.md — § USART1 串口输出
 * @see     doc/learn/newlib-nosys-stdio-retarget.md — §5 printf 与 HAL 选型
 */

#ifndef USART_H
#define USART_H

#include "main.h"

extern UART_HandleTypeDef huart1;

void MX_USART1_UART_Init(void);
void USART1_WriteStr(const char *str);

#endif /* USART_H */
