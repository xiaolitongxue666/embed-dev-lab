/**
 * @file    usart.h
 * @brief   USART1（PA9 TX / PA10 RX）纯寄存器：中断收发 + 回调
 *
 * PA9/PA10：DS5319 标 FT；默认映射，勿 USART1_REMAP（抢 PB6/PB7 I2C1）。
 * 勿用 PA13/PA14（SWD）。
 *
 * TX/RX 经全局环形缓冲；USART1_IRQHandler 覆盖 startup 弱符号。
 * 用户 RX handle 只在 USART1_ProcessRx()（主循环）中调用，ISR 不调 handle。
 *
 * @see     usart.c
 * @see     doc/projects/f103-manual-reg.md — § USART1 与硬件接线
 * @see     doc/hardware/stm32f103c8t6-pinout.md
 */

#ifndef USART_H
#define USART_H

/** 收到一字节后由 USART1_ProcessRx 调用；传 0 取消 */
typedef void (*USART1_RxHandle_t)(unsigned char byte);

void USART1_Init(void);
void USART1_Write(const char *buf, int len);
void USART1_SetRxHandle(USART1_RxHandle_t handle);
void USART1_ProcessRx(void);
void USART1_IRQHandler(void);

#endif /* USART_H */
