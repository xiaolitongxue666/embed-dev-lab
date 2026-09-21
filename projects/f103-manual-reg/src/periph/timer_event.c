/**
 * @file    timer_event.c
 * @brief   软件定时器：ISR 倒计时置 flag，主循环 TakeFlag
 *
 * 每槽：period_ms==0 未注册；cnt 每毫秒减 1，到 0 则 flag=1。
 * 周期模式重装 cnt；单次把 period_ms 清 0，下轮 continue。
 * Register / Unregister / TakeFlag：mrs PRIMASK → cpsid → 改结构 → 原为 0 再 cpsie（可嵌套）。
 *
 * @see     timer_event.h
 */

#include "timer_event.h"

typedef struct {
    unsigned int period_ms;     /* 0=未注册 */
    unsigned int cnt;           /* 剩余毫秒 */
    volatile unsigned char flag; /* ISR 置 1，TakeFlag 清 0 */
    unsigned char mode;         /* ONCE / PERIODIC */
} TimerEvent_t;

static volatile TimerEvent_t s_timer_events[TIMER_EVENT_MAX_NUM];

/** 保存 PRIMASK 后关 IRQ；返回值交给 irq_unlock */
static unsigned int irq_lock(void)
{
    unsigned int primask;

    __asm volatile ("mrs %0, primask" : "=r"(primask));
    __asm volatile ("cpsid i" ::: "memory");
    return primask;
}

/** 仅当调用前未关中断时再开，避免嵌套误开 */
static void irq_unlock(unsigned int primask)
{
    if (primask == 0U) {
        __asm volatile ("cpsie i" ::: "memory");
    }
}

void TimerEvent_OnTick(void)
{
    unsigned int i;

    for (i = 0U; i < (unsigned int)TIMER_EVENT_MAX_NUM; i++) {
        if (s_timer_events[i].period_ms == 0U) {
            continue;
        }

        s_timer_events[i].cnt--;
        if (s_timer_events[i].cnt == 0U) {
            s_timer_events[i].flag = 1U;
            if (s_timer_events[i].mode == (unsigned char)TIMER_MODE_PERIODIC) {
                s_timer_events[i].cnt = s_timer_events[i].period_ms;
            } else {
                s_timer_events[i].period_ms = 0U;
            }
        }
    }
}

unsigned char TimerEvent_Register(unsigned char idx, unsigned int period_ms,
                                  unsigned char mode)
{
    unsigned int primask;

    if ((idx >= (unsigned char)TIMER_EVENT_MAX_NUM) || (period_ms == 0U)) {
        return 1U;
    }
    if ((mode != (unsigned char)TIMER_MODE_ONCE) &&
        (mode != (unsigned char)TIMER_MODE_PERIODIC)) {
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

void TimerEvent_Unregister(unsigned char idx)
{
    unsigned int primask;

    if (idx >= (unsigned char)TIMER_EVENT_MAX_NUM) {
        return;
    }

    primask = irq_lock();
    s_timer_events[idx].period_ms = 0U;
    s_timer_events[idx].cnt = 0U;
    s_timer_events[idx].flag = 0U;
    irq_unlock(primask);
}

unsigned char TimerEvent_TakeFlag(unsigned char idx)
{
    unsigned int primask;
    unsigned char flag;

    if (idx >= (unsigned char)TIMER_EVENT_MAX_NUM) {
        return 0U;
    }

    primask = irq_lock();
    flag = s_timer_events[idx].flag;
    s_timer_events[idx].flag = 0U;
    irq_unlock(primask);
    return flag;
}
