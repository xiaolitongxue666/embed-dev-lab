/**
 * @file    usart.h
 * @brief   USART1（PA9 TX / PA10 RX）HAL 调试口
 *
 * @note    应用日志走 LOG_* → vsnprintf → 一次 HAL_UART_Transmit，**不用 printf**，
 *          故无 syscalls.c / _write。本文件 USART1_WriteStr 仍供底层逐字节发送
 *          （RX 回显）。若要用 printf 须自写 _write，见 f103-manual-reg。
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
void USART1_WriteHex8(uint8_t value);
void USART1_WriteU32(uint32_t value);
void USART1_WriteDec2(uint32_t value);
void USART1_StartRxEcho(void);

#endif /* USART_H */
