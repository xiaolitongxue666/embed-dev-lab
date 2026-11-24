/**
 * @file    startup_stm32f103xb.s
 * @brief   STM32F103xB 启动代码：向量表、Reset_Handler、默认异常处理
 *
 * @target  STM32F103C8T6（Cortex-M3）
 *
 * 本文件是固件中最先执行的代码（上电/复位后硬件读向量表再进 Reset_Handler）。
 *
 * 完整启动链（与本工程源码对应）：
 *
 *   1. 硬件复位
 *        Flash 启动：逻辑 0x00000000 别名到物理 0x08000000
 *        读 g_pfnVectors[0] → 初始 MSP（_estack，见 linker/STM32F103C8_FLASH.ld）
 *        读 g_pfnVectors[1] → PC = Reset_Handler
 *   2. Reset_Handler（本文件）
 *        设 SP → 拷贝 .data（Flash LMA → RAM VMA）→ 清零 .bss
 *   3. bl SystemInit
 *        → projects/.../src/system_stm32f1xx.c
 *        → HSE 8 MHz × PLL×9 → SYSCLK 72 MHz（失败则保持 HSI 8 MHz）
 *   4. bl main
 *        → projects/.../src/main.c
 *        → GPIOC / USART1 / SPI1 / LSM6DS3 …
 *   5. 若 main 返回：b . 死循环（正常固件不应返回）
 *
 * 不使用工具链 crt0（链接 -nostartfiles）；C 运行时最小初始化全部由本文件完成。
 *
 * 向量表含 F103xB 外设 IRQ（USART1 为索引 53，IRQn=37）；未实现的 Handler 为 weak → Default_Handler。
 * USART1_IRQHandler 由 src/usart.c 强符号覆盖。
 *
 * @see     doc/projects/f103-manual-reg.md § 启动与时钟
 * @see     doc/learn/interrupt-vector-table-and-nvic.md
 * @see     doc/learn/stm32-bare-metal-bootstrap.md Q11
 * @see     doc/learn/stm32f103-memory-boot-map.md
 * @see     doc/learn/linker-vma-lma.md
 */

.syntax unified
.cpu cortex-m3
.fpu softvfp
.thumb

.global g_pfnVectors
.global Default_Handler
.global Reset_Handler

/* 链接脚本符号：供 Reset_Handler 初始化 RAM 数据段 */
.word _sidata    /* .data 在 Flash 中的加载地址 */
.word _sdata     /* .data 在 RAM 中的起始地址 */
.word _edata     /* .data 在 RAM 中的结束地址 */
.word _sbss      /* .bss 起始地址 */
.word _ebss      /* .bss 结束地址 */

.equ BootRAM, 0xF108F85F   /* 向量表末字：RAM 启动模式 magic（本工程未用） */

/* -------------------------------------------------------------------------- */
/* 中断向量表（位于 Flash 物理起始 0x08000000，见 STM32F103C8_FLASH.ld）
 * 复位硬件读向量表 +0 → MSP，+4 → PC（Flash 启动：逻辑 0x00000000 别名到此）
 * 详见 doc/learn/stm32f103-memory-boot-map.md                                        */
/* -------------------------------------------------------------------------- */
.section .isr_vector,"a",%progbits
.type g_pfnVectors, %object
.size g_pfnVectors, .-g_pfnVectors
g_pfnVectors:
    .word _estack              /* 0  初始 MSP；满递减栈，此后 push 时 SP 减小 */
    .word Reset_Handler        /* 1  复位入口：本工程最先执行的指令所在函数 */
    .word NMI_Handler          /* 2  不可屏蔽中断 */
    .word HardFault_Handler    /* 3  硬 fault */
    .word MemManage_Handler    /* 4  存储器管理 fault */
    .word BusFault_Handler     /* 5  总线 fault */
    .word UsageFault_Handler   /* 6  用法 fault */
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler          /* 11 SVCall */
    .word DebugMon_Handler     /* 12 调试监视 */
    .word 0
    .word PendSV_Handler       /* 14 PendSV */
    .word SysTick_Handler      /* 15 SysTick */
    /* STM32F103xB 外设 IRQ；USART1 为索引 53（IRQn=37）。须按序填满，不可只插 USART1 */
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
    .word CAN1_SCE_IRQHandler        /* 38  CAN1 SCE（状态变化/错误） */
    .word EXTI9_5_IRQHandler         /* 39  外部中断线 5..9 */
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
    .word USART1_IRQHandler          /* 53  USART1 全局（usart.c 强符号覆盖） */
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
    .word BootRAM                    /* 66  RAM 启动模式 magic @0x108（本工程未用） */

/* -------------------------------------------------------------------------- */
/* Reset_Handler：C 运行时就绪 → SystemInit → main
 *
 * 顺序不可调换：未拷贝 .data / 未清 .bss 前不可进 C；未 SystemInit 前
 * USART1 波特率按 72 MHz 计算会错（若仍停在 HSI 8 MHz）。
 * -------------------------------------------------------------------------- */
.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    ldr r0, =_estack
    mov sp, r0                 /* MSP = RAM 上界；栈向低地址增长（push 减 SP） */

    /* .data：LMA（Flash/_sidata）→ VMA（RAM/_sdata.._edata）搬运，见 doc/learn/linker-vma-lma.md §4.1 */
    ldr r0, =_sdata                      /* 拷贝目标：.data 在 RAM 的 VMA 起始 */
    ldr r1, =_edata                      /* 拷贝目标：.data 在 RAM 的 VMA 结束（不含） */
    ldr r2, =_sidata                     /* 拷贝源：.data 初值在 Flash 的 LMA 起始 */
    movs r3, #0
    b LoopCopyDataInit

CopyDataInit:
    ldr r4, [r2, r3]
    str r4, [r0, r3]
    adds r3, r3, #4

LoopCopyDataInit:
    adds r4, r0, r3
    cmp r4, r1
    bcc CopyDataInit

    /* .bss：仅 VMA，无 LMA；将 _sbss.._ebss 清零，见 doc/learn/linker-vma-lma.md §4.2 */
    ldr r2, =_sbss                       /* .bss 在 RAM 的 VMA 起始 */
    ldr r4, =_ebss                       /* .bss 在 RAM 的 VMA 结束（不含） */
    movs r3, #0
    b LoopFillZerobss

FillZerobss:
    str r3, [r2]
    adds r2, r2, #4

LoopFillZerobss:
    cmp r2, r4
    bcc FillZerobss

    /* 链接解析到 system_stm32f1xx.c::SystemInit；main 内不再调用 */
    bl SystemInit
    /* 链接解析到 main.c::main（LED / USART1 / SPI1 LSM6DS3） */
    bl main
    b .                        /* main 不应返回；若返回则死循环 */

.size Reset_Handler, .-Reset_Handler

/* -------------------------------------------------------------------------- */
/* 默认异常：弱符号，未实现的中断落入此死循环                                  */
/* -------------------------------------------------------------------------- */
.weak Default_Handler
.type Default_Handler, %function
Default_Handler:
    b Default_Handler

.weak NMI_Handler
.thumb_set NMI_Handler, Default_Handler
.weak HardFault_Handler
.thumb_set HardFault_Handler, Default_Handler
.weak MemManage_Handler
.thumb_set MemManage_Handler, Default_Handler
.weak BusFault_Handler
.thumb_set BusFault_Handler, Default_Handler
.weak UsageFault_Handler
.thumb_set UsageFault_Handler, Default_Handler
.weak SVC_Handler
.thumb_set SVC_Handler, Default_Handler
.weak DebugMon_Handler
.thumb_set DebugMon_Handler, Default_Handler
.weak PendSV_Handler
.thumb_set PendSV_Handler, Default_Handler
.weak SysTick_Handler
.thumb_set SysTick_Handler, Default_Handler
.weak WWDG_IRQHandler
.thumb_set WWDG_IRQHandler, Default_Handler
.weak PVD_IRQHandler
.thumb_set PVD_IRQHandler, Default_Handler
.weak TAMPER_IRQHandler
.thumb_set TAMPER_IRQHandler, Default_Handler
.weak RTC_IRQHandler
.thumb_set RTC_IRQHandler, Default_Handler
.weak FLASH_IRQHandler
.thumb_set FLASH_IRQHandler, Default_Handler
.weak RCC_IRQHandler
.thumb_set RCC_IRQHandler, Default_Handler
.weak EXTI0_IRQHandler
.thumb_set EXTI0_IRQHandler, Default_Handler
.weak EXTI1_IRQHandler
.thumb_set EXTI1_IRQHandler, Default_Handler
.weak EXTI2_IRQHandler
.thumb_set EXTI2_IRQHandler, Default_Handler
.weak EXTI3_IRQHandler
.thumb_set EXTI3_IRQHandler, Default_Handler
.weak EXTI4_IRQHandler
.thumb_set EXTI4_IRQHandler, Default_Handler
.weak DMA1_Channel1_IRQHandler
.thumb_set DMA1_Channel1_IRQHandler, Default_Handler
.weak DMA1_Channel2_IRQHandler
.thumb_set DMA1_Channel2_IRQHandler, Default_Handler
.weak DMA1_Channel3_IRQHandler
.thumb_set DMA1_Channel3_IRQHandler, Default_Handler
.weak DMA1_Channel4_IRQHandler
.thumb_set DMA1_Channel4_IRQHandler, Default_Handler
.weak DMA1_Channel5_IRQHandler
.thumb_set DMA1_Channel5_IRQHandler, Default_Handler
.weak DMA1_Channel6_IRQHandler
.thumb_set DMA1_Channel6_IRQHandler, Default_Handler
.weak DMA1_Channel7_IRQHandler
.thumb_set DMA1_Channel7_IRQHandler, Default_Handler
.weak ADC1_2_IRQHandler
.thumb_set ADC1_2_IRQHandler, Default_Handler
.weak USB_HP_CAN1_TX_IRQHandler
.thumb_set USB_HP_CAN1_TX_IRQHandler, Default_Handler
.weak USB_LP_CAN1_RX0_IRQHandler
.thumb_set USB_LP_CAN1_RX0_IRQHandler, Default_Handler
.weak CAN1_RX1_IRQHandler
.thumb_set CAN1_RX1_IRQHandler, Default_Handler
.weak CAN1_SCE_IRQHandler
.thumb_set CAN1_SCE_IRQHandler, Default_Handler
.weak EXTI9_5_IRQHandler
.thumb_set EXTI9_5_IRQHandler, Default_Handler
.weak TIM1_BRK_IRQHandler
.thumb_set TIM1_BRK_IRQHandler, Default_Handler
.weak TIM1_UP_IRQHandler
.thumb_set TIM1_UP_IRQHandler, Default_Handler
.weak TIM1_TRG_COM_IRQHandler
.thumb_set TIM1_TRG_COM_IRQHandler, Default_Handler
.weak TIM1_CC_IRQHandler
.thumb_set TIM1_CC_IRQHandler, Default_Handler
.weak TIM2_IRQHandler
.thumb_set TIM2_IRQHandler, Default_Handler
.weak TIM3_IRQHandler
.thumb_set TIM3_IRQHandler, Default_Handler
.weak TIM4_IRQHandler
.thumb_set TIM4_IRQHandler, Default_Handler
.weak I2C1_EV_IRQHandler
.thumb_set I2C1_EV_IRQHandler, Default_Handler
.weak I2C1_ER_IRQHandler
.thumb_set I2C1_ER_IRQHandler, Default_Handler
.weak I2C2_EV_IRQHandler
.thumb_set I2C2_EV_IRQHandler, Default_Handler
.weak I2C2_ER_IRQHandler
.thumb_set I2C2_ER_IRQHandler, Default_Handler
.weak SPI1_IRQHandler
.thumb_set SPI1_IRQHandler, Default_Handler
.weak SPI2_IRQHandler
.thumb_set SPI2_IRQHandler, Default_Handler
.weak USART1_IRQHandler
.thumb_set USART1_IRQHandler, Default_Handler
.weak USART2_IRQHandler
.thumb_set USART2_IRQHandler, Default_Handler
.weak USART3_IRQHandler
.thumb_set USART3_IRQHandler, Default_Handler
.weak EXTI15_10_IRQHandler
.thumb_set EXTI15_10_IRQHandler, Default_Handler
.weak RTC_Alarm_IRQHandler
.thumb_set RTC_Alarm_IRQHandler, Default_Handler
.weak USBWakeUp_IRQHandler
.thumb_set USBWakeUp_IRQHandler, Default_Handler
