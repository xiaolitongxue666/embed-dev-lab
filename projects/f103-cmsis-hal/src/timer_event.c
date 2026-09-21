/**
 * @file    timer_event.c
 * @brief   软件定时器 + SysTime：ISR 倒计时置 flag
 *
 * OnTick 由 SysTick_Handler 在 HAL_IncTick 之后调用。
 * 不改 uwTick。SysTime_Reset 只清单调 ms/sec。
 *
 * @see     timer_event.h
 */

#include "timer_event.h"

typedef struct {
    uint32_t period_ms;
    uint32_t cnt;
    volatile uint8_t flag;
    uint8_t mode;
} TimerEvent_t;

static volatile TimerEvent_t s_timer_events[TIMER_EVENT_MAX_NUM];
static volatile uint32_t s_sys_ms;
static volatile uint32_t s_sys_sec;

static uint32_t irq_lock(void)
{
    uint32_t primask;

    primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

static void irq_unlock(uint32_t primask)
{
    if (primask == 0U) {
        __enable_irq();
    }
}

void TimerEvent_OnTick(void)
{
    uint32_t i;

    s_sys_ms++;
    if (s_sys_ms >= 1000U) {
        s_sys_ms = 0U;
        s_sys_sec++;
    }

    for (i = 0U; i < (uint32_t)TIMER_EVENT_MAX_NUM; i++) {
        if (s_timer_events[i].period_ms == 0U) {
            continue;
        }

        s_timer_events[i].cnt--;
        if (s_timer_events[i].cnt == 0U) {
            s_timer_events[i].flag = 1U;
            if (s_timer_events[i].mode == (uint8_t)TIMER_MODE_PERIODIC) {
                s_timer_events[i].cnt = s_timer_events[i].period_ms;
            } else {
                s_timer_events[i].period_ms = 0U;
            }
        }
    }
}

uint8_t TimerEvent_Register(uint8_t idx, uint32_t period_ms, uint8_t mode)
{
    uint32_t primask;

    if ((idx >= (uint8_t)TIMER_EVENT_MAX_NUM) || (period_ms == 0U)) {
        return 1U;
    }
    if ((mode != (uint8_t)TIMER_MODE_ONCE) &&
        (mode != (uint8_t)TIMER_MODE_PERIODIC)) {
        return 1U;
    }

    primask = irq_lock();
    s_timer_events[idx].period_ms = period_ms;
    s_timer_events[idx].cnt = period_ms;
    s_timer_events[idx].flag = 0U;
    s_timer_events[idx].mode = mode;
    irq_unlock(primask);
    return 0U;
}

void TimerEvent_Unregister(uint8_t idx)
{
    uint32_t primask;

    if (idx >= (uint8_t)TIMER_EVENT_MAX_NUM) {
        return;
    }

    primask = irq_lock();
    s_timer_events[idx].period_ms = 0U;
    s_timer_events[idx].cnt = 0U;
    s_timer_events[idx].flag = 0U;
    irq_unlock(primask);
}

uint8_t TimerEvent_TakeFlag(uint8_t idx)
{
    uint32_t primask;
    uint8_t flag;

    if (idx >= (uint8_t)TIMER_EVENT_MAX_NUM) {
        return 0U;
    }

    primask = irq_lock();
    flag = s_timer_events[idx].flag;
    s_timer_events[idx].flag = 0U;
    irq_unlock(primask);
    return flag;
}

void SysTime_Get(SysTime_t *t)
{
    SysTime_t snap;

    if (t == NULL) {
        return;
    }

    do {
        snap.sec = s_sys_sec;
        snap.ms = s_sys_ms;
    } while (snap.sec != s_sys_sec);

    t->sec = snap.sec;
    t->ms = snap.ms;
}

void SysTime_Reset(void)
{
    uint32_t primask;

    primask = irq_lock();
    s_sys_ms = 0U;
    s_sys_sec = 0U;
    irq_unlock(primask);
}
