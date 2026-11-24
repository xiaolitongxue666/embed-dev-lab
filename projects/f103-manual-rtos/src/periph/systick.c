/**
 * @file    systick.c
 * @brief   Cortex-M3 SysTick：HCLK=72 MHz 时 1 ms 中断
 *
 * 寄存器：ARMv7-M SysTick（0xE000E010），非 STM32 外设。
 * LOAD = 72000-1 → 72 MHz / 72000 = 1 kHz。异常号 15，非 NVIC 外设 IRQ。
 *
 * 本文件不定义 SysTick_Handler（强符号在 os.c，startup 里的是 WEAK）。
 * SysTime_OnTick 只加毫秒和秒，不画屏、不 printf、不改就绪链表。
 *
 * @see     systick.h
 * @see     os.c
 * @see     doc/learn/interrupt-vector-table-and-nvic.md
 */

#include "systick.h"

#define SYST_CSR (*(volatile unsigned int *)0xE000E010U)
#define SYST_RVR (*(volatile unsigned int *)0xE000E014U)
#define SYST_CVR (*(volatile unsigned int *)0xE000E018U)

#define SYST_CSR_ENABLE    (1U << 0)
#define SYST_CSR_TICKINT   (1U << 1)
#define SYST_CSR_CLKSOURCE (1U << 2)

/** 72 MHz 下 1 ms；处理器时钟，不是 HCLK/8 */
#define SYSTICK_RELOAD_1MS 71999U

/** 单调毫秒，GetMs；与 ms/sec 独立累加 */
static volatile unsigned int s_systick_ms;
/** 秒内 0–999，给 SysTime_Get */
static volatile unsigned int s_sys_ms;
/** 累计秒，给 SysTime_Get */
static volatile unsigned int s_sys_sec;

void SysTime_OnTick(void)
{
    s_systick_ms++;
    s_sys_ms++;
    if (s_sys_ms >= 1000U) {
        s_sys_ms = 0U;
        s_sys_sec++;
    }
}

void SysTick_Init(void)
{
    SYST_CSR = 0U;
    SYST_RVR = SYSTICK_RELOAD_1MS;
    SYST_CVR = 0U;
    s_systick_ms = 0U;
    s_sys_ms = 0U;
    s_sys_sec = 0U;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;
}

unsigned int SysTick_GetMs(void)
{
    return s_systick_ms;
}

void SysTick_SetMs(unsigned int ms)
{
    unsigned int primask;

    /* 三套一起写；cpsid 防止 ISR 插在半中间 */
    __asm volatile ("mrs %0, primask" : "=r"(primask));
    __asm volatile ("cpsid i" ::: "memory");
    s_systick_ms = ms;
    s_sys_sec = ms / 1000U;
    s_sys_ms = ms % 1000U;
    if (primask == 0U) {
        __asm volatile ("cpsie i" ::: "memory");
    }
}

void SysTime_Get(SysTime_t *t)
{
    SysTime_t snap;

    if (t == 0) {
        return;
    }

    /* 复核 sec：ms=999→0 且 sec++ 时只核 ms 会读到撕裂值 */
    do {
        snap.sec = s_sys_sec;
        snap.ms = s_sys_ms;
    } while (snap.sec != s_sys_sec);

    t->sec = snap.sec;
    t->ms = snap.ms;
}
