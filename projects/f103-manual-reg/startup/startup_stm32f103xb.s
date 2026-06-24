/**
 * @file    startup_stm32f103xb.s
 * @brief   STM32F103xB 启动代码：向量表、Reset 处理、默认异常处理
 *
 * @target  STM32F103C8T6（Cortex-M3）
 * @note    Reset_Handler 流程：设栈 → 拷贝 .data → 清零 .bss → SystemInit → main
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

/* -------------------------------------------------------------------------- */
/* 中断向量表（位于 Flash 物理起始 0x08000000，见 STM32F103C8_FLASH.ld）
 * 复位硬件读向量表 +0 → MSP，+4 → PC（Flash 启动：逻辑 0x00000000 别名到此）
 * 详见 doc/learn/stm32f103-memory-boot-map.md                                        */
/* -------------------------------------------------------------------------- */
.section .isr_vector,"a",%progbits
.type g_pfnVectors, %object
.size g_pfnVectors, .-g_pfnVectors
g_pfnVectors:
    .word _estack              /* 0  初始主栈顶 */
    .word Reset_Handler        /* 1  复位 */
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

/* -------------------------------------------------------------------------- */
/* 复位处理：C 运行时环境最小初始化                                            */
/* -------------------------------------------------------------------------- */
.section .text.Reset_Handler
.weak Reset_Handler
.type Reset_Handler, %function
Reset_Handler:
    ldr r0, =_estack
    mov sp, r0                 /* 设置主栈指针 MSP */

    /* 将 .data 从 Flash 拷贝到 RAM */
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata
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

    /* 清零 .bss */
    ldr r2, =_sbss
    ldr r4, =_ebss
    movs r3, #0
    b LoopFillZerobss

FillZerobss:
    str r3, [r2]
    adds r2, r2, #4

LoopFillZerobss:
    cmp r2, r4
    bcc FillZerobss

    bl SystemInit              /* 系统时钟（system_stm32f1xx.c） */
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
