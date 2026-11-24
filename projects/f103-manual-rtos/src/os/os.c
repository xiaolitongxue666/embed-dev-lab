/**
 * @file    os.c
 * @brief   就绪位图、延时链表、SysTick 拍板、SVC 之前的启动
 *
 * SysTick 只决定“要不要换人”，并挂起 PendSV。真正换栈在 port.c。
 * 调度器启动前，SysTick 只调用 SysTime_OnTick，给 BMP280 的忙等和 OLED 时钟用。
 *
 * 时间片减到 0 必须重装。链表上只剩当前任务时不挂 PendSV，否则每毫秒空切一次。
 *
 * @see     port.c
 * @see     systick.h
 * @see     doc/learn/manual-rtos-context-switch.md
 * @see     doc/projects/f103-manual-rtos.md
 */

#include "os.h"

#include "systick.h"

/* SCB */
#define SCB_ICSR  (*(volatile unsigned int *)0xE000ED04U)
#define SCB_SHPR3 ((volatile unsigned char *)0xE000ED20U)

#define SCB_ICSR_PENDSVSET (1U << 28)

/** SHPR3 字节 2 是 PendSV，字节 3 是 SysTick。F103 只实现高 4 位，0xF0 即优先级 15 */
#define SCB_SHPR_LOWEST 0xF0U

static tcb_t *ready_list[OS_MAX_PRIORITIES];
static tcb_t *delay_list;
static unsigned int ready_bitmap;

tcb_t *current_tcb;
volatile unsigned int os_running;

static tcb_t tcb_pool[OS_MAX_TASKS];
static unsigned int stack_pool[OS_MAX_TASKS][OS_TASK_STACK_WORDS]
    __attribute__((aligned(8)));
static unsigned int task_count;

static void idle_task(void)
{
    /* 用户任务都在延时时还有人可跑。本内核不删除任务，idle 不负责回收内存 */
    while (1) {
        __asm volatile("wfi");
    }
}

static void pendsv_request(void)
{
    /* 写 1 挂起。多次写仍然只挂起一次，外设中断返回后切到当时最新的任务 */
    SCB_ICSR = SCB_ICSR_PENDSVSET;
}

static unsigned int highest_priority(void)
{
    /* ready_bitmap 非 0 才许调用。bit N 表示优先级 N 的链表非空 */
    return 31U - (unsigned int)__builtin_clz(ready_bitmap);
}

static void ready_add(tcb_t *task)
{
    unsigned int prio;
    tcb_t *head;

    prio = task->priority;
    head = ready_list[prio];
    if (head == 0) {
        task->next = task;
        task->prev = task;
        ready_list[prio] = task;
    } else {
        /* 插到尾：head->prev 是当前尾 */
        task->next = head;
        task->prev = head->prev;
        head->prev->next = task;
        head->prev = task;
    }
    ready_bitmap |= (1U << prio);
}

static void ready_remove(tcb_t *task)
{
    unsigned int prio;

    prio = task->priority;
    if (task->next == task) {
        ready_list[prio] = 0;
        ready_bitmap &= ~(1U << prio);
    } else {
        task->prev->next = task->next;
        task->next->prev = task->prev;
        if (ready_list[prio] == task) {
            ready_list[prio] = task->next;
        }
    }
}

unsigned int os_enter_critical(void)
{
    unsigned int primask;

    __asm volatile("mrs %0, primask" : "=r"(primask));
    __asm volatile("cpsid i" ::: "memory");
    return primask;
}

void os_exit_critical(unsigned int primask)
{
    if (primask == 0U) {
        __asm volatile("cpsie i" ::: "memory");
    }
}

unsigned int os_scheduler_running(void)
{
    return os_running;
}

__attribute__((used)) unsigned int os_stack_free_words(const tcb_t *task)
{
    const unsigned int *word;
    unsigned int free_words;

    if ((task == 0) || (task->stack_base == 0)) {
        return 0U;
    }

    free_words = 0U;
    for (word = task->stack_base; word < task->sp; word++) {
        if (*word != OS_STACK_PAINT) {
            break;
        }
        free_words++;
    }
    return free_words;
}

void os_init(void)
{
    unsigned int i;

    for (i = 0U; i < OS_MAX_PRIORITIES; i++) {
        ready_list[i] = 0;
    }
    ready_bitmap = 0U;
    delay_list = 0;
    current_tcb = 0;
    os_running = 0U;
    task_count = 0U;
    (void)task_create(idle_task, 0U);
}

tcb_t *task_create(os_task_fn entry, unsigned int priority)
{
    unsigned int primask;
    tcb_t *task;
    unsigned int *base;
    unsigned int *stk;
    unsigned int i;

    if ((entry == 0) || (task_count >= OS_MAX_TASKS) ||
        (priority >= OS_MAX_PRIORITIES)) {
        return 0;
    }

    primask = os_enter_critical();

    task = &tcb_pool[task_count];
    base = stack_pool[task_count];
    for (i = 0U; i < OS_TASK_STACK_WORDS; i++) {
        base[i] = OS_STACK_PAINT;
    }

    /*
     * 栈向下长。高地址先放硬件异常帧，再放软件保存的 R11..R4，sp 指着 R4。
     * 第一次被 SVC/PendSV 恢复时，和普通切回来的任务是同一条路径。
     * 栈帧里的 LR 会进任务的 LR 寄存器；Handler 自己的 LR 才决定用 PSP 返回。
     * 任务函数不能 return。
     */
    stk = base + OS_TASK_STACK_WORDS;
    *(--stk) = 0x01000000U;             /* xPSR：Thumb 位必须为 1 */
    *(--stk) = (unsigned int)entry;     /* PC */
    *(--stk) = 0xFFFFFFFD;              /* 任务 LR，不是这次异常的 EXC_RETURN */
    *(--stk) = 0U;                      /* R12 */
    *(--stk) = 0U;                      /* R3 */
    *(--stk) = 0U;                      /* R2 */
    *(--stk) = 0U;                      /* R1 */
    *(--stk) = 0U;                      /* R0 */
    for (i = 0U; i < 8U; i++) {
        *(--stk) = 0U;                  /* R11 .. R4 */
    }

    task->sp = stk;
    task->priority = priority;
    task->ticks_remain = OS_TIME_SLICE_TICKS;
    task->delay_ticks = 0U;
    task->delay_next = 0;
    task->stack_base = base;

    task_count++;
    ready_add(task);

    os_exit_critical(primask);
    return task;
}

tcb_t *schedule_next(void)
{
    unsigned int prio;
    tcb_t *chosen;

    if (ready_bitmap == 0U) {
        return current_tcb;
    }

    prio = highest_priority();
    if ((current_tcb == 0) || (current_tcb->delay_ticks > 0U)) {
        return ready_list[prio];
    }

    /* 抢占：不把被抢占任务从链表头拨走。时间片若刚好用尽，只重装，不轮转 */
    if (prio > current_tcb->priority) {
        if (current_tcb->ticks_remain == 0U) {
            current_tcb->ticks_remain = OS_TIME_SLICE_TICKS;
        }
        return ready_list[prio];
    }

    if ((current_tcb->ticks_remain == 0U) && (current_tcb->priority == prio)) {
        current_tcb->ticks_remain = OS_TIME_SLICE_TICKS;
        if (current_tcb->next != current_tcb) {
            chosen = current_tcb->next;
            ready_list[prio] = chosen;
            chosen->ticks_remain = OS_TIME_SLICE_TICKS;
            return chosen;
        }
        return current_tcb;
    }

    return ready_list[prio];
}

void os_delay(unsigned int ticks)
{
    unsigned int primask;

    if ((ticks == 0U) || (os_running == 0U) || (current_tcb == 0)) {
        return;
    }

    primask = os_enter_critical();
    current_tcb->delay_ticks = ticks;
    ready_remove(current_tcb);
    current_tcb->delay_next = delay_list;
    delay_list = current_tcb;
    pendsv_request();
    os_exit_critical(primask);
}

static void delay_list_on_tick(void)
{
    tcb_t **link;

    link = &delay_list;
    while (*link != 0) {
        tcb_t *task;

        task = *link;
        if (task->delay_ticks > 0U) {
            task->delay_ticks--;
        }
        if (task->delay_ticks == 0U) {
            *link = task->delay_next;
            task->delay_next = 0;
            task->ticks_remain = OS_TIME_SLICE_TICKS;
            ready_add(task);
        } else {
            link = &task->delay_next;
        }
    }
}

void SysTick_Handler(void)
{
    unsigned int hp;

    /* 启动前也要走，BMP280 忙等和 OLED 的秒都靠这套计数 */
    SysTime_OnTick();
    if (os_running == 0U) {
        return;
    }

    delay_list_on_tick();

    if ((current_tcb != 0) && (current_tcb->ticks_remain > 0U)) {
        current_tcb->ticks_remain--;
    }

    if ((ready_bitmap == 0U) || (current_tcb == 0)) {
        return;
    }

    hp = highest_priority();
    if (hp > current_tcb->priority) {
        pendsv_request();
        return;
    }

    if (current_tcb->ticks_remain == 0U) {
        if ((current_tcb->next != current_tcb) &&
            (current_tcb->priority == hp)) {
            pendsv_request();
        } else {
            /* 这一级只有自己：重装时间片，不要空切 */
            current_tcb->ticks_remain = OS_TIME_SLICE_TICKS;
        }
    }
}

void os_start(void)
{
    if (ready_bitmap == 0U) {
        return;
    }

    current_tcb = ready_list[highest_priority()];

    /* 外设 IRQ 的数值更小，仍能打断切换。SVC 保持复位优先级，高于这两条 */
    SCB_SHPR3[2] = SCB_SHPR_LOWEST;
    SCB_SHPR3[3] = SCB_SHPR_LOWEST;

    __asm volatile("svc #0");

    while (1) {
    }
}
