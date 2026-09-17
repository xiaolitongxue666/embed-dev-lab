/**
 * @file    system_stm32f1xx.h
 * @brief   SystemInit 声明（手写实现，不链接官方 CMSIS Device）
 *
 * SystemInit 由 startup_stm32f103xb.s::Reset_Handler 在 main 前调用：
 *   Reset_Handler →（.data / .bss）→ bl SystemInit → bl main
 *
 * @see     system_stm32f1xx.c
 * @see     doc/projects/f103-manual-reg.md § 启动与时钟
 */

#ifndef __SYSTEM_STM32F1XX_H
#define __SYSTEM_STM32F1XX_H

extern void SystemInit(void);

#endif /* __SYSTEM_STM32F1XX_H */
