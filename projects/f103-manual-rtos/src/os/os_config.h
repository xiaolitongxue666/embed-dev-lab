/**
 * @file    os_config.h
 * @brief   手写调度器容量与节拍
 *
 * 优先级数值越大越高，0 留给 idle。栈按字计。
 * 状态任务的 LOG_D 会进 printf / vsnprintf，1024 字节会写穿进相邻任务栈
 * （实测水位约 1028 字节，显示任务因此 INVPC）。384 字 = 1536 字节。
 *
 * @see     os.h
 * @see     doc/learn/manual-rtos-context-switch.md
 */

#ifndef OS_CONFIG_H
#define OS_CONFIG_H

/** 优先级 0–7 */
#define OS_MAX_PRIORITIES 8U
/** 含 idle 在内的 TCB / 栈池个数 */
#define OS_MAX_TASKS 8U
/** 每个任务栈的字数（32 位） */
#define OS_TASK_STACK_WORDS 384U
/** SysTick 频率，与 72 MHz 下重装值 71999 对应 */
#define OS_TICK_HZ 1000U
/** 同优先级时间片，单位 tick。减到 0 后必须重装，否则每拍都挂 PendSV */
#define OS_TIME_SLICE_TICKS 1U

/** 建栈时填充，水位从低地址往上数还剩几个这个值 */
#define OS_STACK_PAINT 0xA5A5A5A5U

#endif /* OS_CONFIG_H */
