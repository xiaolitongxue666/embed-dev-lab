/**
 * @file    os.h
 * @brief   手写抢占调度：TCB、就绪链表、延时、PendSV 入口
 *
 * 没有队列、信号量和任务删除。任务状态不单设字段：
 * 在就绪链表上就是就绪，在延时链表上就是在等 tick。
 *
 * 中断里不要调用 os_delay / task_create。外设 ISR 只置标志。
 *
 * @see     os.c
 * @see     port.c
 * @see     doc/learn/manual-rtos-context-switch.md
 */

#ifndef OS_H
#define OS_H

#include "os_config.h"

typedef struct tcb {
    unsigned int *sp;           /* 必须是第一个字段：PendSV 用 [tcb] 当栈指针 */
    struct tcb *next;           /* 同优先级就绪循环链表 */
    struct tcb *prev;
    struct tcb *delay_next;     /* 延时单向链表 */
    unsigned int priority;      /* 0–7，越大越优先；idle 为 0 */
    unsigned int ticks_remain;  /* 当前时间片剩余 tick */
    unsigned int delay_ticks;   /* 非 0 表示不在就绪链表里 */
    unsigned int *stack_base;   /* 栈最低地址，水位从这里往上扫 */
} tcb_t;

typedef void (*os_task_fn)(void);

/**
 * @brief  清空就绪/延时链表，并创建优先级 0 的 idle（wfi）
 */
void os_init(void);

/**
 * @brief  从池里取 TCB 和栈，伪造异常栈帧，挂进就绪链表
 * @return 成功返回 TCB；任务数或优先级超限返回 0
 * @note   任务函数不能 return
 */
tcb_t *task_create(os_task_fn entry, unsigned int priority);

/**
 * @brief  选最高优先级任务，把 SysTick/PendSV 设为最低，SVC 启动后不再返回
 */
void os_start(void);

/**
 * @brief  当前任务离开就绪链表，挂起 PendSV，醒来前至少等 ticks 个节拍
 * @note   ticks 为 0，或调度器还没启动时直接返回
 */
void os_delay(unsigned int ticks);

/** 保存 PRIMASK 并关中断。链表改动必须包在这对调用里 */
unsigned int os_enter_critical(void);
void os_exit_critical(unsigned int primask);

/** SVC 切到第一个任务之后为 1。在此之前 SysTick 只加毫秒 */
unsigned int os_scheduler_running(void);

/**
 * @brief  从栈底数连续的 OS_STACK_PAINT，得到还没被用过的字数
 * @note   不写进周期日志。符号加 used，避免 --gc-sections 丢掉
 */
unsigned int os_stack_free_words(const tcb_t *task);

/** PendSV 正在运行的任务。port.c 按这个符号名访问 */
extern tcb_t *current_tcb;

/** SVC 末尾置 1。port.c 按这个符号名写 */
extern volatile unsigned int os_running;

/**
 * @brief  选下一个任务。只许 PendSV 调用
 * @note   更高优先级直接取该链表头，不旋转被抢占者。
 *         同优先级且时间片用尽、链表上还有别人，才把链表头拨到 current->next。
 */
tcb_t *schedule_next(void);

#endif /* OS_H */
