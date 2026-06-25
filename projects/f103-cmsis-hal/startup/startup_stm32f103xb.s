/**
  *************** (C) COPYRIGHT 2017 STMicroelectronics ************************
  * @file      startup_stm32f103xb.s
  * @author    MCD Application Team
  * @brief     STM32F103xB Devices vector table for Atollic toolchain.
  *            This module performs:
  *                - Set the initial SP
  *                - Set the initial PC == Reset_Handler,
  *                - Set the vector table entries with the exceptions ISR address
  *                - Configure the clock system   
  *                - Branches to main in the C library (which eventually
  *                  calls main()).
  *            After Reset the Cortex-M3 processor is in Thread mode,
  *            priority is Privileged, and the Stack is set to Main.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017-2021 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  * @note    embed-dev-lab 维护说明（中文）：
  *          Reset 流程（CMSIS 顺序）：SystemInit → 拷贝 .data → 清零 .bss → main
  *          初始 MSP 由向量表第 0 项 _estack 提供，硬件复位时自动加载，无需在
  *          Reset_Handler 中再 mov sp。
  *          已跳过 __libc_init_array（nosys 裸机，避免 _init 链接错误）。
  *          符号 _sidata/_sdata/_edata/_sbss/_ebss 由 STM32F103XB_FLASH.ld 定义。
  * @see     doc/learn/linker-vma-lma.md
  * @see     doc/learn/stm32f103-memory-boot-map.md
  ******************************************************************************
  */

  .syntax unified
  .cpu cortex-m3
  .fpu softvfp
  .thumb

.global g_pfnVectors
.global Default_Handler

/* --------------------------------------------------------------------------
 * 链接脚本符号（STM32F103XB_FLASH.ld 导出）
 * 供 Reset_Handler 在 C 运行时环境就绪前，完成 RAM 中 .data/.bss 的初始化。
 * -------------------------------------------------------------------------- */
/* .data 初值在 Flash 中的加载地址（LMA）；拷贝源起点 */
.word _sidata
/* .data 在 RAM 中的起始地址（VMA）；拷贝目标起点 */
.word _sdata
/* .data 在 RAM 中的结束地址（VMA，不含）；拷贝目标终点 */
.word _edata
/* .bss  在 RAM 中的起始地址（VMA）；清零循环起点 */
.word _sbss
/* .bss  在 RAM 中的结束地址（VMA，不含）；清零循环终点 */
.word _ebss

.equ  BootRAM, 0xF108F85F    /* 中密度器件从 RAM 启动时的引导字（本 demo 从 Flash 启动，未使用） */

/* --------------------------------------------------------------------------
 * Reset_Handler：复位后第一条执行的代码（向量表 +4 项指向此处）
 * CMSIS 顺序：SystemInit → .data 搬运 → .bss 清零 → main
 * 本 demo 系统时钟在 main.c 的 SystemClock_Config()（HAL）中配置，SystemInit 仅做复位默认化。
 * -------------------------------------------------------------------------- */
  .section .text.Reset_Handler
  .weak Reset_Handler
  .type Reset_Handler, %function
Reset_Handler:

/* SystemInit：CMSIS 系统默认化（向量表等）；PLL/72 MHz 见 main.c HAL 配置 */
    bl  SystemInit

/* ---------- .data 搬运：Flash(LMA/_sidata) → RAM(VMA/_sdata.._edata) ---------- */
/* r0 = 目标地址起点(_sdata)，r1 = 目标终点(_edata)，r2 = 源地址(_sidata)，r3 = 偏移 */
  ldr r0, =_sdata
  ldr r1, =_edata
  ldr r2, =_sidata
  movs r3, #0
  b LoopCopyDataInit

CopyDataInit:
  ldr r4, [r2, r3]          /* 从 Flash 读 4 字节初值 */
  str r4, [r0, r3]          /* 写入 RAM 对应 VMA 位置 */
  adds r3, r3, #4           /* 偏移 +4（字对齐拷贝） */

LoopCopyDataInit:
  adds r4, r0, r3           /* 当前目标地址 = _sdata + offset */
  cmp r4, r1                /* 是否已达 _edata（不含） */
  bcc CopyDataInit          /* 未完成则继续拷贝 */

/* ---------- .bss 清零：仅 VMA，Flash 镜像无内容；C 未初始化全局变量依赖此步 ---------- */
/* r2 = 清零起点(_sbss)，r4 = 清零终点(_ebss)，r3 = 写入值 0 */
  ldr r2, =_sbss
  ldr r4, =_ebss
  movs r3, #0
  b LoopFillZerobss

FillZerobss:
  str  r3, [r2]             /* 写 0 到当前 .bss 地址 */
  adds r2, r2, #4

LoopFillZerobss:
  cmp r2, r4                /* 是否已达 _ebss（不含） */
  bcc FillZerobss

/* nosys 裸机无 C++ 全局构造；若调用 __libc_init_array 会链接 _init 失败 */
/* bl __libc_init_array */
  bl main                   /* 进入应用 main.c */
  bx lr                     /* main 返回则沿 LR 返回（通常不应发生） */
.size Reset_Handler, .-Reset_Handler

/* --------------------------------------------------------------------------
 * Default_Handler：未实现的中断/异常统一落入此死循环，便于调试器定位
 * -------------------------------------------------------------------------- */
    .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b Infinite_Loop
  .size Default_Handler, .-Default_Handler

/******************************************************************************
 * g_pfnVectors：Cortex-M3 中断向量表
 * 须位于 Flash 物理起始 0x08000000（linker .isr_vector 段 > FLASH 首段）
 * 复位硬件：从 +0 读 MSP(_estack)，从 +4 读 PC(Reset_Handler)
 * BOOT0=0 时 CPU 将 0x00000000 别名映射到 0x08000000，故向量表必须在 Flash 开头
 ******************************************************************************/
  .section .isr_vector,"a",%progbits
  .type g_pfnVectors, %object
  .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:

  .word _estack                  /*  0  初始主栈指针 MSP（满递减栈上界） */
  .word Reset_Handler            /*  1  复位 */
  .word NMI_Handler              /*  2  不可屏蔽中断 */
  .word HardFault_Handler        /*  3  硬 fault */
  .word MemManage_Handler        /*  4  存储器管理 fault（Cortex-M3 可选） */
  .word BusFault_Handler         /*  5  总线 fault（Cortex-M3 可选） */
  .word UsageFault_Handler       /*  6  用法 fault（Cortex-M3 可选） */
  .word 0                          /*  7  保留 */
  .word 0                          /*  8  保留 */
  .word 0                          /*  9  保留 */
  .word 0                          /* 10  保留 */
  .word SVC_Handler              /* 11  SVCall（RTOS 上下文切换常用） */
  .word DebugMon_Handler         /* 12  调试监视 */
  .word 0                          /* 13  保留 */
  .word PendSV_Handler           /* 14  PendSV（RTOS 上下文切换常用） */
  .word SysTick_Handler          /* 15  SysTick 系统节拍 */
  .word WWDG_IRQHandler           /* 16  窗口看门狗 */
  .word PVD_IRQHandler            /* 17  可编程电压检测 */
  .word TAMPER_IRQHandler       /* 18  侵入检测 */
  .word RTC_IRQHandler           /* 19  RTC 全局中断 */
  .word FLASH_IRQHandler         /* 20  Flash 全局中断 */
  .word RCC_IRQHandler           /* 21  RCC 全局中断 */
  .word EXTI0_IRQHandler         /* 22  EXTI 线 0 */
  .word EXTI1_IRQHandler         /* 23  EXTI 线 1 */
  .word EXTI2_IRQHandler         /* 24  EXTI 线 2 */
  .word EXTI3_IRQHandler         /* 25  EXTI 线 3 */
  .word EXTI4_IRQHandler         /* 26  EXTI 线 4 */
  .word DMA1_Channel1_IRQHandler /* 27  DMA1 通道 1 */
  .word DMA1_Channel2_IRQHandler /* 28  DMA1 通道 2 */
  .word DMA1_Channel3_IRQHandler /* 29  DMA1 通道 3 */
  .word DMA1_Channel4_IRQHandler /* 30  DMA1 通道 4 */
  .word DMA1_Channel5_IRQHandler /* 31  DMA1 通道 5 */
  .word DMA1_Channel6_IRQHandler /* 32  DMA1 通道 6 */
  .word DMA1_Channel7_IRQHandler /* 33  DMA1 通道 7 */
  .word ADC1_2_IRQHandler        /* 34  ADC1/ADC2 全局 */
  .word USB_HP_CAN1_TX_IRQHandler/* 35  USB 高优先级 / CAN1 TX */
  .word USB_LP_CAN1_RX0_IRQHandler /* 36 USB 低优先级 / CAN1 RX0 */
  .word CAN1_RX1_IRQHandler      /* 37  CAN1 RX1 */
  .word CAN1_SCE_IRQHandler      /* 38  CAN1 SCE */
  .word EXTI9_5_IRQHandler       /* 39  EXTI 线 [9:5] */
  .word TIM1_BRK_IRQHandler      /* 40  TIM1 刹车 */
  .word TIM1_UP_IRQHandler       /* 41  TIM1 更新 */
  .word TIM1_TRG_COM_IRQHandler  /* 42  TIM1 触发与通信 */
  .word TIM1_CC_IRQHandler       /* 43  TIM1 捕获比较 */
  .word TIM2_IRQHandler         /* 44  TIM2 全局 */
  .word TIM3_IRQHandler         /* 45  TIM3 全局 */
  .word TIM4_IRQHandler         /* 46  TIM4 全局 */
  .word I2C1_EV_IRQHandler      /* 47  I2C1 事件 */
  .word I2C1_ER_IRQHandler      /* 48  I2C1 错误 */
  .word I2C2_EV_IRQHandler      /* 49  I2C2 事件 */
  .word I2C2_ER_IRQHandler      /* 50  I2C2 错误 */
  .word SPI1_IRQHandler         /* 51  SPI1 全局 */
  .word SPI2_IRQHandler         /* 52  SPI2 全局 */
  .word USART1_IRQHandler       /* 53  USART1 全局 */
  .word USART2_IRQHandler       /* 54  USART2 全局 */
  .word USART3_IRQHandler       /* 55  USART3 全局 */
  .word EXTI15_10_IRQHandler    /* 56  EXTI 线 [15:10] */
  .word RTC_Alarm_IRQHandler    /* 57  RTC 闹钟 */
  .word USBWakeUp_IRQHandler    /* 58  USB 唤醒 */
  .word 0                          /* 59  保留 */
  .word 0                          /* 60  保留 */
  .word 0                          /* 61  保留 */
  .word 0                          /* 62  保留 */
  .word 0                          /* 63  保留 */
  .word 0                          /* 64  保留 */
  .word 0                          /* 65  保留 */
  .word BootRAM                   /* 66  中密度 RAM 启动引导字 @ 向量偏移 0x108 */

/*******************************************************************************
 * 弱符号别名：用户未实现的中断/异常自动指向 Default_Handler
 * 在应用层定义同名强符号函数即可覆盖（如 void SysTick_Handler(void) { ... }）
 ******************************************************************************/

  .weak NMI_Handler
  .thumb_set NMI_Handler,Default_Handler

  .weak HardFault_Handler
  .thumb_set HardFault_Handler,Default_Handler

  .weak MemManage_Handler
  .thumb_set MemManage_Handler,Default_Handler

  .weak BusFault_Handler
  .thumb_set BusFault_Handler,Default_Handler

  .weak UsageFault_Handler
  .thumb_set UsageFault_Handler,Default_Handler

  .weak SVC_Handler
  .thumb_set SVC_Handler,Default_Handler

  .weak DebugMon_Handler
  .thumb_set DebugMon_Handler,Default_Handler

  .weak PendSV_Handler
  .thumb_set PendSV_Handler,Default_Handler

  .weak SysTick_Handler
  .thumb_set SysTick_Handler,Default_Handler

  .weak WWDG_IRQHandler
  .thumb_set WWDG_IRQHandler,Default_Handler

  .weak PVD_IRQHandler
  .thumb_set PVD_IRQHandler,Default_Handler

  .weak TAMPER_IRQHandler
  .thumb_set TAMPER_IRQHandler,Default_Handler

  .weak RTC_IRQHandler
  .thumb_set RTC_IRQHandler,Default_Handler

  .weak FLASH_IRQHandler
  .thumb_set FLASH_IRQHandler,Default_Handler

  .weak RCC_IRQHandler
  .thumb_set RCC_IRQHandler,Default_Handler

  .weak EXTI0_IRQHandler
  .thumb_set EXTI0_IRQHandler,Default_Handler

  .weak EXTI1_IRQHandler
  .thumb_set EXTI1_IRQHandler,Default_Handler

  .weak EXTI2_IRQHandler
  .thumb_set EXTI2_IRQHandler,Default_Handler

  .weak EXTI3_IRQHandler
  .thumb_set EXTI3_IRQHandler,Default_Handler

  .weak EXTI4_IRQHandler
  .thumb_set EXTI4_IRQHandler,Default_Handler

  .weak DMA1_Channel1_IRQHandler
  .thumb_set DMA1_Channel1_IRQHandler,Default_Handler

  .weak DMA1_Channel2_IRQHandler
  .thumb_set DMA1_Channel2_IRQHandler,Default_Handler

  .weak DMA1_Channel3_IRQHandler
  .thumb_set DMA1_Channel3_IRQHandler,Default_Handler

  .weak DMA1_Channel4_IRQHandler
  .thumb_set DMA1_Channel4_IRQHandler,Default_Handler

  .weak DMA1_Channel5_IRQHandler
  .thumb_set DMA1_Channel5_IRQHandler,Default_Handler

  .weak DMA1_Channel6_IRQHandler
  .thumb_set DMA1_Channel6_IRQHandler,Default_Handler

  .weak DMA1_Channel7_IRQHandler
  .thumb_set DMA1_Channel7_IRQHandler,Default_Handler

  .weak ADC1_2_IRQHandler
  .thumb_set ADC1_2_IRQHandler,Default_Handler

  .weak USB_HP_CAN1_TX_IRQHandler
  .thumb_set USB_HP_CAN1_TX_IRQHandler,Default_Handler

  .weak USB_LP_CAN1_RX0_IRQHandler
  .thumb_set USB_LP_CAN1_RX0_IRQHandler,Default_Handler

  .weak CAN1_RX1_IRQHandler
  .thumb_set CAN1_RX1_IRQHandler,Default_Handler

  .weak CAN1_SCE_IRQHandler
  .thumb_set CAN1_SCE_IRQHandler,Default_Handler

  .weak EXTI9_5_IRQHandler
  .thumb_set EXTI9_5_IRQHandler,Default_Handler

  .weak TIM1_BRK_IRQHandler
  .thumb_set TIM1_BRK_IRQHandler,Default_Handler

  .weak TIM1_UP_IRQHandler
  .thumb_set TIM1_UP_IRQHandler,Default_Handler

  .weak TIM1_TRG_COM_IRQHandler
  .thumb_set TIM1_TRG_COM_IRQHandler,Default_Handler

  .weak TIM1_CC_IRQHandler
  .thumb_set TIM1_CC_IRQHandler,Default_Handler

  .weak TIM2_IRQHandler
  .thumb_set TIM2_IRQHandler,Default_Handler

  .weak TIM3_IRQHandler
  .thumb_set TIM3_IRQHandler,Default_Handler

  .weak TIM4_IRQHandler
  .thumb_set TIM4_IRQHandler,Default_Handler

  .weak I2C1_EV_IRQHandler
  .thumb_set I2C1_EV_IRQHandler,Default_Handler

  .weak I2C1_ER_IRQHandler
  .thumb_set I2C1_ER_IRQHandler,Default_Handler

  .weak I2C2_EV_IRQHandler
  .thumb_set I2C2_EV_IRQHandler,Default_Handler

  .weak I2C2_ER_IRQHandler
  .thumb_set I2C2_ER_IRQHandler,Default_Handler

  .weak SPI1_IRQHandler
  .thumb_set SPI1_IRQHandler,Default_Handler

  .weak SPI2_IRQHandler
  .thumb_set SPI2_IRQHandler,Default_Handler

  .weak USART1_IRQHandler
  .thumb_set USART1_IRQHandler,Default_Handler

  .weak USART2_IRQHandler
  .thumb_set USART2_IRQHandler,Default_Handler

  .weak USART3_IRQHandler
  .thumb_set USART3_IRQHandler,Default_Handler

  .weak EXTI15_10_IRQHandler
  .thumb_set EXTI15_10_IRQHandler,Default_Handler

  .weak RTC_Alarm_IRQHandler
  .thumb_set RTC_Alarm_IRQHandler,Default_Handler

  .weak USBWakeUp_IRQHandler
  .thumb_set USBWakeUp_IRQHandler,Default_Handler


