# 手写调度：上下文切换

这篇只讲 `f103-manual-rtos` 里那套 SysTick + PendSV 为什么能让一个核看起来在同时跑多个任务。单步演示见 [调度器介绍页](f103-manual-rtos-scheduler.html)。接线、任务周期和排错在 [f103-manual-rtos 工程说明](../projects/f103-manual-rtos.md)。向量号和 NVIC 优先级位见 [中断向量表与 NVIC](interrupt-vector-table-and-nvic.md)。

源码符号：`PendSV_Handler`、`SVC_Handler`、`SysTick_Handler`、`tcb_t`、`idle_task`。

## F103 上这些机制都有

STM32F103 是 Cortex-M3。下面几件事硬件直接支持，本工程都用上了：

| 机制 | 在本工程里的用途 |
|------|------------------|
| MSP / PSP | 中断用主栈 MSP，任务用进程栈 PSP |
| PendSV | 真正换栈 |
| SVC | 从 `main` 的 MSP 启动第一个任务 |
| `STMDB/LDMIA {R4-R11}` | M3 一条指令保存或恢复这 8 个寄存器。M0 没有这种低寄存器列表形式 |
| SysTick | 24 位递减计数器，72 MHz 下重装 71999，1 ms 一拍 |
| `CLZ` | `__builtin_clz` 从就绪位图找出最高优先级，和任务个数无关 |

不链接 CMSIS，也不使用标准库的 `stm32f10x.h`。优先级写 `SCB_SHPR3`，挂起 PendSV 写 `SCB_ICSR` 的 bit28。

## PendSV

PendSV（Pendable Service Call，可挂起的服务调用）是专门用来换任务的异常。

需要切换时，调度器不在当前中断里换栈，只把 PendSV 挂起：写 `SCB_ICSR` 的 PendSV 挂起位。等于先记一笔，等更急的中断做完再换人。

它被写成最低优先级。串口、DMA、按键这些外设中断数值更小，可以先做完。PendSV 不会插进这些中断中间。若在别的中断处理期间又挂了一次，标志还是那一位，返回后只切一次，切到当时最新的就绪任务。

所以换人分两步：SysTick（或 `os_delay`）负责拍板，`PendSV_Handler` 负责执行。

## R4–R11

R4 到 R11 是 CPU 里的通用寄存器。C 源码里写这些名字不会通过，编译器按 AAPCS 自己分配。这 8 个是被调函数要保存的寄存器。

汇编可以直接点名。`STMDB R0!, {R4-R11}` 的意思是：把 R4 到 R11 按高到低存进 R0 指向的内存，R0 每次先减再写。

异常进入时，硬件已经把 R0–R3、R12、LR、PC、xPSR 压进**当前正在用的栈**。任务跑在 PSP 上时，这 8 个字进的是任务栈。`PendSV_Handler` 因此只再保存 R4–R11，然后改 PSP。两边加起来才是完整现场。

## TCB

TCB 是 Task Control Block（任务控制块）。一个任务对内核来说不只是函数入口，还要有一份档案。本工程的档案是 `tcb_t`：

- `sp`：上次停下来时栈停在哪。这是第一项，PendSV 用 `[tcb]` 直接当栈指针
- `priority`：0 到 7，数值越大越优先。idle 固定是 0
- `ticks_remain`：当前时间片还剩几拍
- `delay_ticks`：还要睡几拍。非 0 表示它不在就绪链表里
- `next` / `prev`：同一优先级的循环双向链表
- `delay_next`：延时单向链表
- `stack_base`：这块栈的最低地址，用来数还剩多少填充字

没有单独的状态字段。在就绪链表上就是就绪，在延时链表上就是在等 tick。没有任务名，也没有删除任务。不要把 FreeRTOS 的 `TCB_t` 里那几十个成员当成这里已经有了。

## Idle

Idle 是空闲任务。`os_init` 自动创建，优先级 0。

调度器的规矩是：系统在跑，就必须至少有一个任务可以执行。用户任务都在 `os_delay` 里时，CPU 不能没有下一条指令。`idle_task` 就是这个兜底：循环里执行 `wfi`，睡到下一个中断，用来省电。

FreeRTOS 的 idle 还会在空闲时释放已删除任务的内存。本内核没有删除任务，idle 不做回收。

## tick 到了是不是立刻切换

是中断，但是两步。

SysTick 计数到 0，进入 `SysTick_Handler`。这个函数不做换栈。它做三件事：

1. `SysTime_OnTick`：单调毫秒加一，秒内毫秒满 1000 就进位。OLED 时钟和 BMP280 忙等读的是这套数。
2. 延时链表每个任务减 1，减到 0 的放回就绪链表，并重装时间片。
3. 看当前时间片是否用尽，或就绪位图里是否有更高优先级。需要换人时只挂起 PendSV。

真正保存 R4–R11、调用 `schedule_next`、改 PSP 的是随后的 `PendSV_Handler`。这样换栈这点开销不会插在串口或 DMA 中断中间。

调度器还没启动时（`os_running == 0`），SysTick 只做第 1 步。否则 BMP280 初始化还在 `main` 里忙等，链表却可能被改掉。`os_running` 在 `SVC_Handler` 把 PSP 装好之后才写成 1。

同优先级且这一级只有当前任务时，时间片用尽只重装，不挂 PendSV。不重装的话，每毫秒都会空切一次。

## 上下文切换是否必须汇编

这条抢占路径必须汇编。

C 编译器按调用约定占用 R4–R11，也写不出“把这 8 个寄存器存进当前任务栈、把 PSP 指到下一个任务、用 `0xFFFFFFFD` 从异常返回”。`0xFFFFFFFD` 是异常返回值：回到线程模式，并且用 PSP。它放在 Handler 自己的 LR 里，不是任务栈帧里那个 LR。

`PendSV_Handler` 要 `BL schedule_next`。`BL` 会改写 LR，把异常返回值冲掉，所以调用前先把 LR 压进主栈 MSP。任务现场仍在 PSP 上。

`setjmp` / `longjmp` 只能保存一部分环境，不够做这种带异常返回的抢占切换。本工程不用它们。

协作式调度可以在任务自己调用某个函数时才换人。本工程是 SysTick 到点就抢占，所以核心切换在 `port.c` 的裸函数里。

## 栈写穿看起来像非法返回

任务栈在 `.bss` 里一块挨着一块，向低地址生长。高优先级任务的栈底紧贴着下一块任务的栈顶。`printf` / `vsnprintf` 的调用栈若越过本任务的 `stack_base`，写掉的是下一块任务上次保存的异常帧。那块任务再被换回来时，硬件弹出的 PC 不是代码地址。CFSR 的 UFSR.INVPC（`0x00040000`）因此被置位；UsageFault 默认没开，HFSR bit30 把它收成 HardFault。Handler 的 LR 若是 `0xFFFFFFFD`，这 8 个字在 PSP 上。

2026-11-24：状态任务的 `LOG_D` 水位约 1028 字节，1024 字节栈写进显示任务。`OS_TASK_STACK_WORDS` 现为 384（1536 字节）。不要为了省 RAM 改回 256。
