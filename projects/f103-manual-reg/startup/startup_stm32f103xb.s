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
 * @see     doc/projects/f103-manual-reg.md § 启动与时钟
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
