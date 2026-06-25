/**
  *************** (C) COPYRIGHT 2017 STMicroelectronics ************************
  * @file      startup_stm32f103xb.s
  * @author    MCD Application Team
  * @brief     STM32F103xB Devices vector table for GCC toolchain.
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
  * @note    embed-dev-lab / f103-cmsis-hal 说明：
  *          - 目标芯片 STM32F103C8T6（Medium-density F103xB，64K Flash / 20K RAM）
  *          - 向量表位于 Flash 0x08000000（链接脚本 .isr_vector，见 linker/STM32F103XB_FLASH.ld）
  *          - Reset 流程：SystemInit → 拷贝 .data（LMA→VMA）→ 清零 .bss → main
  *          - 已跳过 __libc_init_array（nosys 裸机，避免 _init 链接错误）
  *          - 本 demo 72 MHz 时钟在 main.c 的 SystemClock_Config() 中由 HAL 配置；
  *            SystemInit 仅做向量表等 CMSIS 复位默认化（见 src/system_stm32f1xx.c）
  *          - SysTick_Handler 在 src/stm32f1xx_it.c 中实现，供 HAL_IncTick / HAL_Delay
  *          - 详见 doc/learn/stm32f103-memory-boot-map.md、linker-vma-lma.md
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* -------------------------------------------------------------------------- */
/* 汇编目标与 ABI 设置                                                         */
/* -------------------------------------------------------------------------- */
  .syntax unified          /* Thumb-2 统一语法 */
  .cpu cortex-m3           /* ARMv7-M，无 FPU */
  .fpu softvfp             /* 软浮点 ABI 占位（Cortex-M3 无硬件 FPU） */
  .thumb                   /* 指令集：Thumb / Thumb-2 */

.global g_pfnVectors       /* 向量表起始，链接脚本 KEEP(.isr_vector) 固定于 Flash 首 */
.global Default_Handler

/* -------------------------------------------------------------------------- */
/* 链接脚本符号（STM32F103XB_FLASH.ld 定义，供 Reset_Handler 初始化 RAM）      */
/* 详见 doc/learn/linker-vma-lma.md §4                                        */
/* -------------------------------------------------------------------------- */
.word _sidata              /* .data 初值在 Flash 中的加载地址（LMA） */
.word _sdata               /* .data 在 RAM 中的起始地址（VMA） */
.word _edata               /* .data 在 RAM 中的结束地址（不含） */
.word _sbss                /* .bss 在 RAM 中的起始地址（VMA，无 LMA） */
.word _ebss                /* .bss 在 RAM 中的结束地址（不含） */

.equ BootRAM, 0xF108F85F   /* 向量表末字：RAM 启动模式 magic（本 demo 未使用） */

/* -------------------------------------------------------------------------- */
/* Reset_Handler：复位后 C 运行时最小初始化                                     */
/* 硬件复位时：从 g_pfnVectors[0] 加载 MSP，从 g_pfnVectors[1] 跳转至此         */
/* CMSIS 顺序：先 SystemInit，再 .data/.bss，最后 main（与 manual-reg 略有不同）*/
/* -------------------------------------------------------------------------- */
  .section .text.Reset_Handler
  .weak Reset_Handler
  .type Reset_Handler, %function
Reset_Handler:

/* 1) CMSIS 系统默认化：向量表相关寄存器；本 demo 不在此配置 PLL */
    bl  SystemInit

/* 2) .data 搬运：Flash(LMA/_sidata) → RAM(VMA/_sdata.._edata)，逐字拷贝 */
  ldr r0, =_sdata           /* r0 = 拷贝目标起始（RAM） */
  ldr r1, =_edata           /* r1 = 拷贝目标结束（不含） */
  ldr r2, =_sidata          /* r2 = 拷贝源起始（Flash） */
  movs r3, #0               /* r3 = 字节偏移，从 0 递增 */
  b LoopCopyDataInit

CopyDataInit:
  ldr r4, [r2, r3]          /* 读 Flash 初值 */
  str r4, [r0, r3]          /* 写入 RAM */
  adds r3, r3, #4           /* 下一字（4 字节） */

LoopCopyDataInit:
  adds r4, r0, r3           /* 当前目标地址 = _sdata + offset */
  cmp r4, r1                /* 未达 _edata 则继续 */
  bcc CopyDataInit

/* 3) .bss 清零：上电 RAM 内容未定义，须将 _sbss.._ebss 写 0 */
  ldr r2, =_sbss            /* r2 = 清零起始 */
  ldr r4, =_ebss            /* r4 = 清零结束（不含） */
  movs r3, #0               /* 填充值 0 */
  b LoopFillZerobss

FillZerobss:
  str  r3, [r2]             /* *addr = 0 */
  adds r2, r2, #4           /* 下一字 */

LoopFillZerobss:
  cmp r2, r4                /* 未达 _ebss 则继续 */
  bcc FillZerobss

/* 裸机 nosys 无 C++ 全局构造，跳过 __libc_init_array（避免链接 _init） */
/* bl __libc_init_array */

/* 4) 进入应用；main 返回后 fall-through 到 bx lr（通常不应返回） */
  bl main
  bx lr

.size Reset_Handler, .-Reset_Handler

/* -------------------------------------------------------------------------- */
/* Default_Handler：未实现的中断/异常落入此死循环                              */
/* 各 Handler 为 weak 别名；在 C 文件中定义同名强符号即可覆盖                   */
/* -------------------------------------------------------------------------- */
    .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
  b Infinite_Loop
  .size Default_Handler, .-Default_Handler

/* -------------------------------------------------------------------------- */
/* 中断向量表 g_pfnVectors                                                    */
/* 必须位于 Flash 物理起始（本工程 0x08000000，链接脚本 .isr_vector）           */
/* Cortex-M3 规定：表项 N 的地址 = 表基址 + 4×N；表项 0=MSP，1=Reset，2+=异常/IRQ */
/* -------------------------------------------------------------------------- */
  .section .isr_vector,"a",%progbits
  .type g_pfnVectors, %object
  .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:

  .word _estack                    /*  0  初始主栈指针 MSP；满递减栈，_estack=0x20005000 */
  .word Reset_Handler              /*  1  复位向量 → Reset_Handler */
  .word NMI_Handler                /*  2  不可屏蔽中断 NMI */
  .word HardFault_Handler          /*  3  硬 Fault（总线/用法等严重错误） */
  .word MemManage_Handler          /*  4  存储器管理 Fault（MPU，F103 无 MPU 时少见） */
  .word BusFault_Handler           /*  5  总线 Fault */
  .word UsageFault_Handler         /*  6  用法 Fault（未定义指令、非对齐访问等） */
  .word 0                          /*  7  保留 */
  .word 0                          /*  8  保留 */
  .word 0                          /*  9  保留 */
  .word 0                          /* 10  保留 */
  .word SVC_Handler                /* 11  SVCall（系统调用） */
  .word DebugMon_Handler           /* 12  调试监视器 */
  .word 0                          /* 13  保留 */
  .word PendSV_Handler             /* 14  PendSV（常用于 RTOS 上下文切换） */
  .word SysTick_Handler            /* 15  SysTick；本 demo 在 stm32f1xx_it.c → HAL_IncTick */

  /* --- STM32F103xB 外设 IRQ（IRQn 与 stm32f103xb.h 一致） --- */
  .word WWDG_IRQHandler            /* 16  窗口看门狗 */
  .word PVD_IRQHandler             /* 17  可编程电压检测 PVD */
  .word TAMPER_IRQHandler          /* 18  侵入检测 Tamper */
  .word RTC_IRQHandler             /* 19  RTC 全局中断 */
  .word FLASH_IRQHandler           /* 20  Flash 操作完成 */
  .word RCC_IRQHandler             /* 21  RCC 时钟安全/就绪等 */
  .word EXTI0_IRQHandler           /* 22  外部中断线 0 */
  .word EXTI1_IRQHandler           /* 23  外部中断线 1 */
  .word EXTI2_IRQHandler           /* 24  外部中断线 2 */
  .word EXTI3_IRQHandler           /* 25  外部中断线 3 */
  .word EXTI4_IRQHandler           /* 26  外部中断线 4 */
  .word DMA1_Channel1_IRQHandler   /* 27  DMA1 通道 1 */
  .word DMA1_Channel2_IRQHandler   /* 28  DMA1 通道 2 */
  .word DMA1_Channel3_IRQHandler   /* 29  DMA1 通道 3 */
  .word DMA1_Channel4_IRQHandler   /* 30  DMA1 通道 4 */
  .word DMA1_Channel5_IRQHandler   /* 31  DMA1 通道 5 */
  .word DMA1_Channel6_IRQHandler   /* 32  DMA1 通道 6 */
  .word DMA1_Channel7_IRQHandler   /* 33  DMA1 通道 7 */
  .word ADC1_2_IRQHandler          /* 34  ADC1 / ADC2 */
  .word USB_HP_CAN1_TX_IRQHandler  /* 35  USB 高优先级 / CAN1 TX */
  .word USB_LP_CAN1_RX0_IRQHandler /* 36  USB 低优先级 / CAN1 RX0 */
  .word CAN1_RX1_IRQHandler        /* 37  CAN1 RX1 */
  .word CAN1_SCE_IRQHandler         /* 38  CAN1 SCE（状态变化/错误） */
  .word EXTI9_5_IRQHandler          /* 39  外部中断线 5..9 */
  .word TIM1_BRK_IRQHandler        /* 40  TIM1 刹车 */
  .word TIM1_UP_IRQHandler         /* 41  TIM1 更新 */
  .word TIM1_TRG_COM_IRQHandler    /* 42  TIM1 触发/换相 */
  .word TIM1_CC_IRQHandler         /* 43  TIM1 捕获/比较 */
  .word TIM2_IRQHandler            /* 44  TIM2 全局 */
  .word TIM3_IRQHandler            /* 45  TIM3 全局 */
  .word TIM4_IRQHandler            /* 46  TIM4 全局 */
  .word I2C1_EV_IRQHandler         /* 47  I2C1 事件 */
  .word I2C1_ER_IRQHandler         /* 48  I2C1 错误 */
  .word I2C2_EV_IRQHandler         /* 49  I2C2 事件 */
  .word I2C2_ER_IRQHandler         /* 50  I2C2 错误 */
  .word SPI1_IRQHandler            /* 51  SPI1 全局 */
  .word SPI2_IRQHandler            /* 52  SPI2 全局 */
  .word USART1_IRQHandler          /* 53  USART1 全局 */
  .word USART2_IRQHandler          /* 54  USART2 全局 */
  .word USART3_IRQHandler          /* 55  USART3 全局 */
  .word EXTI15_10_IRQHandler       /* 56  外部中断线 10..15 */
  .word RTC_Alarm_IRQHandler       /* 57  RTC 闹钟 */
  .word USBWakeUp_IRQHandler       /* 58  USB 唤醒 */
  .word 0                          /* 59  保留 */
  .word 0                          /* 60  保留 */
  .word 0                          /* 61  保留 */
  .word 0                          /* 62  保留 */
  .word 0                          /* 63  保留 */
  .word 0                          /* 64  保留 */
  .word 0                          /* 65  保留 */
  .word BootRAM                    /* 66  RAM 启动模式 magic @0x108（本 demo 未用） */

/* -------------------------------------------------------------------------- */
/* 弱符号别名：未在 C/汇编中实现的 Handler 均指向 Default_Handler             */
/* .weak + .thumb_set：允许在 stm32f1xx_it.c 等文件中用强符号覆盖             */
/* 本 demo 仅实现 SysTick_Handler；其余外设 IRQ 触发时将进入 Default_Handler  */
/* -------------------------------------------------------------------------- */

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

