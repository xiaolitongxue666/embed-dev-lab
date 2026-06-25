/**
 * @file    stm32f1xx_it.h
 * @brief   Cortex-M3 异常/中断处理函数声明
 *
 * @note    向量表在 startup_stm32f103xb.s；未列出的外设 IRQ 仍指向 Default_Handler。
 *          本 demo 仅 SysTick 有实际逻辑（HAL_IncTick），其余为占位或死循环。
 *
 * 与 startup 弱符号关系：
 *   startup 中 .weak XXX / .thumb_set XXX,Default_Handler
 *   本文件提供强符号实现，链接时覆盖弱符号。
 */

#ifndef STM32F1XX_IT_H
#define STM32F1XX_IT_H

#include "main.h"

void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

#endif /* STM32F1XX_IT_H */
