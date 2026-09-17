/**
 * @file    systick.c
 * @brief   Cortex-M3 SysTick：HCLK=72 MHz 时 1 ms 中断
 *
 * 寄存器：ARMv7-M SysTick（0xE000E010），非 STM32 外设。
 * LOAD = 72000-1 → 72 MHz / 72000 = 1 kHz。异常号 15，非 NVIC 外设 IRQ。
 *
 * @see     systick.h
 * @see     doc/learn/interrupt-vector-table-and-nvic.md
 */

#include "systick.h"

#define SYST_CSR (*(volatile unsigned int *)0xE000E010U)
#define SYST_RVR (*(volatile unsigned int *)0xE000E014U)
#define SYST_CVR (*(volatile unsigned int *)0xE000E018U)

#define SYST_CSR_ENABLE    (1U << 0)
#define SYST_CSR_TICKINT   (1U << 1)
#define SYST_CSR_CLKSOURCE (1U << 2)

/** 72 MHz 下 1 ms */
#define SYSTICK_RELOAD_1MS 71999U

static volatile unsigned int s_systick_ms;

void SysTick_Handler(void)
{
    s_systick_ms++;
}

void SysTick_Init(void)
{
    SYST_CSR = 0U;
    SYST_RVR = SYSTICK_RELOAD_1MS;
    SYST_CVR = 0U;
    s_systick_ms = 0U;
    SYST_CSR = SYST_CSR_CLKSOURCE | SYST_CSR_TICKINT | SYST_CSR_ENABLE;
}

unsigned int SysTick_GetMs(void)
{
    return s_systick_ms;
}

void SysTick_SetMs(unsigned int ms)
{
    s_systick_ms = ms;
}
