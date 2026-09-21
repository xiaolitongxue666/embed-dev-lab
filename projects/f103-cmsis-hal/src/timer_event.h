/**
 * @file    timer_event.h
 * @brief   软件定时事件：HAL SysTick 1 ms 倒计时，到期置 flag
 *
 * 中断只计数、置位；画屏 / 串口 / I2C 全在主循环。
 * 关中断用 CMSIS __disable_irq，不手写 cpsid。
 *
 * @see     timer_event.c
 */

#ifndef TIMER_EVENT_H
#define TIMER_EVENT_H

#include "main.h"

enum { TIMER_EVENT_MAX_NUM = 4 };

enum {
    TIMER_EVT_OLED_CLOCK = 0, /* 50ms：读 SysTime，画 00:00:00:00 */
    TIMER_EVT_LED_LOG         /* 1000ms：LED + 旋钮/按键日志 + 温度 */
};

enum {
    TIMER_MODE_ONCE = 0,
    TIMER_MODE_PERIODIC
};

typedef struct {
    uint32_t ms;
    uint32_t sec;
} SysTime_t;

void TimerEvent_OnTick(void);

uint8_t TimerEvent_Register(uint8_t idx, uint32_t period_ms, uint8_t mode);
void TimerEvent_Unregister(uint8_t idx);
uint8_t TimerEvent_TakeFlag(uint8_t idx);

void SysTime_Get(SysTime_t *t);
void SysTime_Reset(void);

#endif /* TIMER_EVENT_H */
