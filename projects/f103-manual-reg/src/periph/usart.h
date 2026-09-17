/**
 * @file    usart.h
 * @brief   USART1（PA9 TX / PA10 RX）纯寄存器：DMA 收发 + 空闲定界
 *
 * PA9/PA10：DS5319 标 FT；默认映射，勿 USART1_REMAP（抢 PB6/PB7 I2C1）。
 * 勿用 PA13/PA14（SWD）。
 *
 * TX：DMA1 CH4；RX：DMA1 CH5 + CR1.IDLEIE。只开 DMA TCIE，HTIE 保持 0。
 * ISR 不调用户 handle。
 *
 * @see     usart.c
 * @see     doc/projects/f103-manual-reg.md — § USART1 与硬件接线
 * @see     doc/hardware/stm32f103c8t6-pinout.md
 */

#ifndef USART_H
#define USART_H

/** 收完一帧后由 USART1_ProcessRx 调用；传 0 取消 */
typedef void (*USART1_RxHandle_t)(const unsigned char *data, unsigned int len);

void USART1_Init(void);
void USART1_Write(const char *buf, int len);
void USART1_SetRxHandle(USART1_RxHandle_t handle);
void USART1_ProcessRx(void);
void USART1_IRQHandler(void);
void DMA1_Channel4_IRQHandler(void);
void DMA1_Channel5_IRQHandler(void);

#endif /* USART_H */
