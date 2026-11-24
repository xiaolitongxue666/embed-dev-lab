/**
 * @file    systick.h
 * @brief   SysTick 1 ms 时基（处理器时钟，72 MHz 下 1 kHz）
 *
 * 硬件节拍由 SysTick_Init 打开。SysTick_Handler 在 os.c：
 * 每拍先 SysTime_OnTick，调度器启动后再处理延时和时间片。
 * HSE 失败则节拍变慢（重装值仍按 72 MHz）。
 *
 * 两套时间，ISR 只加计数，不画屏、不 printf：
 *   - 单调毫秒 SysTick_GetMs：一直加，约 49 天回绕；BMP280 延时用无符号减法
 *   - SysTime_t.ms / .sec：秒内 0–999，满 1000 清零并 sec++；OLED 四段钟用这个
 *
 * 硬件仍是 CLKSOURCE + LOAD=71999，不是 HCLK/8。
 *
 * @see     systick.c
 * @see     os.c
 * @see     doc/projects/f103-manual-rtos.md
 */

#ifndef SYSTICK_H
#define SYSTICK_H

typedef struct {
    unsigned int ms;  /* 秒内毫秒 0–999 */
    unsigned int sec; /* 累计秒 */
} SysTime_t;

void SysTick_Init(void);

/** SysTick_Handler 每拍调用一次。不要在别的地方改这两套计数 */
void SysTime_OnTick(void);

/** 单调累计毫秒；给 bmp280_delay_ms 等做 (now - start) */
unsigned int SysTick_GetMs(void);

/**
 * @brief  同时改单调毫秒与 ms/sec，避免只改一套导致 OLED 对不上
 * @note   OLED Probe 成功后 SetMs(0) 从 00:00:00:00 起算
 */
void SysTick_SetMs(unsigned int ms);

/**
 * @brief  主循环读「现在几点」：栈上快照，复核 sec，避免 ms 回绕时撕裂
 * @param  t  输出；空指针直接返回
 */
void SysTime_Get(SysTime_t *t);

#endif /* SYSTICK_H */
