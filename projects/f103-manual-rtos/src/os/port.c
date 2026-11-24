/**
 * @file    port.c
 * @brief   PendSV 换栈、SVC 启动第一个任务
 *
 * Cortex-M3 进入异常时，硬件已经把 R0–R3、R12、LR、PC、xPSR 压进当前栈。
 * 这里只手工保存被调函数负责的 R4–R11，再改 PSP。
 * C 写不出这几条，所以这两个 Handler 是裸函数汇编。
 *
 * BL schedule_next 会冲掉 LR 里的 EXC_RETURN，所以调用前把 LR 压进主栈（MSP）。
 * 任务栈是 PSP，中断栈是 MSP，两边不混。
 *
 * startup 里这三个名字是 WEAK。本文件提供 PendSV / SVC 的强符号，
 * SysTick_Handler 的强符号在 os.c。不要再写第二份。
 *
 * @see     os.h
 * @see     doc/learn/manual-rtos-context-switch.md
 * @see     doc/learn/interrupt-vector-table-and-nvic.md
 */

#include "os.h"

__attribute__((naked)) void PendSV_Handler(void)
{
    __asm volatile(
        "   mrs     r0, psp              \n"
        "   cbz     r0, 1f               \n" /* PSP 为 0：还没有任务现场 */
        "   stmdb   r0!, {r4-r11}        \n"
        "   ldr     r1, =current_tcb     \n"
        "   ldr     r2, [r1]             \n"
        "   str     r0, [r2]             \n" /* current->sp，sp 是 TCB 第一项 */

        "1: push    {lr}                 \n" /* 留下 EXC_RETURN，BL 会改 LR */
        "   bl      schedule_next        \n"
        "   pop     {lr}                 \n"
        "   ldr     r1, =current_tcb     \n"
        "   str     r0, [r1]             \n" /* current_tcb = next */
        "   ldr     r0, [r0]             \n" /* next->sp */
        "   ldmia   r0!, {r4-r11}        \n"
        "   msr     psp, r0              \n"
        "   orr     lr, lr, #0x04        \n" /* 返回线程模式并用 PSP */
        "   bx      lr                   \n");
}

__attribute__((naked)) void SVC_Handler(void)
{
    __asm volatile(
        "   ldr     r0, =current_tcb     \n"
        "   ldr     r1, [r0]             \n"
        "   ldr     r0, [r1]             \n" /* current->sp */
        "   ldmia   r0!, {r4-r11}        \n"
        "   msr     psp, r0              \n"
        "   ldr     r2, =os_running      \n" /* 置位之后 SysTick 才许挂 PendSV */
        "   movs    r3, #1               \n"
        "   str     r3, [r2]             \n"
        "   orr     lr, lr, #0x04        \n" /* 从 MSP 的 Handler 返回到 PSP 线程 */
        "   bx      lr                   \n");
}
