/**
 * @file    key.h
 * @brief   PB13 按键：EXTI13 双沿（IRQn 40）
 *
 * @see     key.c
 * @see     doc/reference/stm32f103/md/topics/dma1-irq-map.md
 */

#ifndef KEY_H
#define KEY_H

void Key_ExtiInit(void);

/** 取走一次边沿；1=有边沿待处理 */
unsigned char Key_TakeEdge(void);

void EXTI15_10_IRQHandler(void);

#endif /* KEY_H */
