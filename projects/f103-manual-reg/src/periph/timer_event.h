/**
 * @file    timer_event.h
 * @brief   软件定时事件：SysTick 1 ms 倒计时，到期置 flag，主循环 TakeFlag
 *
 * 中断只计数、置位；画屏 / printf / I2C 全在主循环。
 * 槽位数组 static，不对外暴露。关中断用手写 cpsid，不包含 CMSIS。
 * TIMER_EVENT_MAX_NUM 用 enum（编译期常量，可作数组长）；C 的 const 不行。
 *
 * 主循环配合：Register → 每 1ms OnTick 到期置 flag → TakeFlag 后再 SysTime_Get / 画屏。
 * OLED 整屏 I2C 慢，画钟走 50ms 槽，不进 SysTick_Handler。
 *
 * @see     timer_event.c
 * @see     systick.c
 */

#ifndef TIMER_EVENT_H
#define TIMER_EVENT_H

enum { TIMER_EVENT_MAX_NUM = 4 };

enum {
    TIMER_EVT_OLED_CLOCK = 0, /* 50ms：读 SysTime，画 00:00:00:00 */
    TIMER_EVT_LED_LOG         /* 1000ms：LED + 旋钮/按键日志 + 温度 */
};

enum {
    TIMER_MODE_ONCE = 0,
    TIMER_MODE_PERIODIC
};

/** 仅 SysTick_Handler：未注册跳过；cnt--，到 0 置 flag */
void TimerEvent_OnTick(void);

/**
 * @brief  注册定时槽（填 period/cnt/mode，清 flag）
 * @retval 0 成功，1 idx/周期/模式非法
 */
unsigned char TimerEvent_Register(unsigned char idx, unsigned int period_ms,
                                  unsigned char mode);

/** period_ms=0，下轮 OnTick 跳过该槽 */
void TimerEvent_Unregister(unsigned char idx);

/**
 * @brief  读并清零，避免主循环直接摸全局 flag
 * @retval 1 已到期，0 未到期或 idx 非法
 */
unsigned char TimerEvent_TakeFlag(unsigned char idx);

#endif /* TIMER_EVENT_H */
