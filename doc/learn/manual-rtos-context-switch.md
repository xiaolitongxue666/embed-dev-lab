# 手写调度：上下文切换

这篇只讲 `f103-manual-rtos` 里那套 SysTick(System Tick: 内核节拍定时器) + PendSV(Pendable Service Call: 可挂起服务调用) 为什么能让一个核看起来在同时跑多个任务。单步演示见 [调度器介绍页](f103-manual-rtos-scheduler.html)。时间片见 [时间片与双栈](f103-manual-rtos-timeslice.html)。PendSV 逐条指令见 [PendSV 逐指令](f103-manual-rtos-pendsv.html)。接线、任务周期和排错在 [f103-manual-rtos 工程说明](../projects/f103-manual-rtos.md)。向量号和 NVIC(Nested Vectored Interrupt Controller: 嵌套向量中断控制器) 优先级位见 [中断向量表与 NVIC](interrupt-vector-table-and-nvic.md)。

源码符号：`PendSV_Handler`、`SVC_Handler`、`SysTick_Handler`、`tcb_t`、`idle_task`。

## F103(STM32F103: 本工程的目标芯片) 上这些机制都有

STM32F103 是 Cortex-M3。下面几件事硬件直接支持，本工程都用上了：

| 机制 | 在本工程里的用途 |
|------|------------------|
| MSP(Main Stack Pointer: 主栈指针) / PSP(Process Stack Pointer: 进程栈指针) | 中断用主栈 MSP(Main Stack Pointer: 主栈指针)，任务用进程栈 PSP(Process Stack Pointer: 进程栈指针) |
| PendSV(Pendable Service Call: 可挂起服务调用) | 真正换栈 |
| SVC(Supervisor Call: 监督调用) | 从 `main` 的 MSP(Main Stack Pointer: 主栈指针) 启动第一个任务 |
| `STMDB(Store Multiple Decrement Before: 先减地址再存多个寄存器)/LDMIA(Load Multiple Increment After: 先取多个寄存器再增地址) {R4-R11}` | Cortex-M3 一条指令保存或恢复这 8 个寄存器。Cortex-M0 没有这种低寄存器列表形式 |
| SysTick(System Tick: 内核节拍定时器) | 24 位递减计数器，72 MHz 下重装 71999，1 ms 一拍 |
| `CLZ(Count Leading Zeros: 数前导零)` | `__builtin_clz` 从就绪位图找出最高优先级，和任务个数无关 |

不链接 CMSIS(Cortex Microcontroller Software Interface Standard: 内核软件接口标准)，也不使用标准库的 `stm32f10x.h`。优先级写 `SCB_SHPR3`。SCB(System Control Block: 系统控制块) 的 SHPR3(System Handler Priority Register 3: 系统异常优先级寄存器 3) 放着系统异常的优先级。挂起 PendSV(Pendable Service Call: 可挂起服务调用) 写 `SCB_ICSR` 的 bit28。ICSR(Interrupt Control and State Register: 中断控制及状态寄存器) 的这一位只表示“记一笔，稍后做”。

## 从 main 到第一次任务，再到以后每次切换

可以把 MSP(Main Stack Pointer: 主栈指针) 和 PSP(Process Stack Pointer: 进程栈指针) 想成两本记事本。中断和内核记在 MSP 上，普通任务记在 PSP 上。全芯片只有一个 PSP 寄存器。每个任务有自己的一块栈；切换时把该任务 TCB(Task Control Block: 任务控制块) 里的 `sp` 装进这个寄存器。

复位后 CPU(Central Processing Unit: 中央处理器) 在 Thread 模式，活动栈是 MSP。MSP 的初值来自向量表第一个字，链接脚本里的 `_estack`（`0x20005000`）。PSP 复位后没有可用的初值，必须由软件先写入。

`os_start` 选好第一个任务后执行 `svc #0`。SVC(Supervisor Call: 监督调用) 是同步异常，立刻进入 Handler 模式，活动栈强制换成 MSP。`SVC_Handler` 只做这一次起跑：从 TCB 弹出 R4–R11，`MSR PSP`，把 `os_running` 写成 1，再把 LR(Link Register: 链接寄存器，存放返回地址) 的 bit2 置上，然后 `BX LR`。返回 Thread 时，硬件按异常返回值 `0xFFFFFFFD` 改用 PSP。第一个任务从这里开始跑。

有的移植在 SVC 处理函数里写 CONTROL(Control Register: 控制寄存器) 的 SPSEL 位，再执行 `ISB`。本工程的 `port.c` 不写 CONTROL，也不执行 `ISB`。切到 PSP 靠的是 `MSR PSP` 加上这条异常返回值。

SVC 之后的换人不再走 SVC。SysTick(System Tick: 内核节拍定时器) 或 `os_delay` 只挂起 PendSV(Pendable Service Call: 可挂起服务调用)。PendSV 的优先级是 `0xF0`，等高优先级中断做完才运行。进入 PendSV 时，硬件已经把 R0–R3、R12、LR、PC(Program Counter: 程序计数器)、xPSR(Program Status Register: 程序状态寄存器) 压进当时正在用的栈。任务在跑时，这 8 个字进的是它的 PSP 栈。随后活动栈换成 MSP，Handler 自己的压栈和任务现场分开。

PendSV 再把 R4–R11 压进同一块 PSP 栈，把此时的指针写入 `current_tcb->sp`，再从下一个 TCB 取出 `sp` 装回 PSP。时间片到 0 且同级只剩当前任务时，只重装时间片，不挂 PendSV。

搬运这 8 个寄存器的方向：

- STMDB(Store Multiple Decrement Before: 先减地址再存) `R0!, {R4-R11}`：8 个字，共减 32 字节。最低编号的 R4 落在最低地址（旧指针减 32），R11 在这块的最高地址（旧指针减 4）。栈向下长，所以是先减再存。
- LDMIA(Load Multiple Increment After: 先取出再增地址) 方向相反。R4 从最低地址弹出，指针加回 32。
- `PUSH` / `POP` 是对当前活动 SP 做 STMDB / LDMIA 的别名。Handler 里活动 SP 是 MSP，所以保存任务现场必须写明 PSP，不能用会作用在 MSP 上的 `PUSH {R4-R11}`。

Cortex-M3 的 Thumb-2 可以在一条 `STMDB` / `LDMIA` 里带上 R8–R11。Cortex-M0 的多寄存器存取只能直接带 R0–R7。R8–R11 要先搬到低寄存器再存，恢复时用来中转的低寄存器最后恢复。本工程是 Cortex-M3，用一条 `{R4-R11}`。

FreeRTOS 的 GCC `ARM_CM3` 端口在 `BL vTaskSwitchContext` 前后用 BASEPRI(Base Priority Register: 优先级屏蔽阈值)，并在 MSP 上保存 `R3` 和 `R14`。源码见 [FreeRTOS ARM_CM3 port.c](https://github.com/FreeRTOS/FreeRTOS-Kernel/blob/main/portable/GCC/ARM_CM3/port.c)。本工程的 PendSV 只 `PUSH {LR}`。临界区在 `os.c` 里用 PRIMASK(Priority Mask Register: 优先级屏蔽，关掉可配置中断)，不在 PendSV 里写 BASEPRI。

## PendSV(Pendable Service Call: 可挂起服务调用)

PendSV(Pendable Service Call: 可挂起服务调用) 是专门用来换任务的异常。

需要切换时，调度器不在当前中断里换栈，只把 PendSV(Pendable Service Call: 可挂起服务调用) 挂起：写 `SCB_ICSR` 的 PendSV(Pendable Service Call: 可挂起服务调用) 挂起位。等于先记一笔，等更急的中断做完再换人。

它被写成最低优先级。串口、DMA(Direct Memory Access: 直接存储器访问)、按键这些外设中断数值更小，可以先做完。PendSV(Pendable Service Call: 可挂起服务调用) 不会插进这些中断中间。若在别的中断处理期间又挂了一次，标志还是那一位，返回后只切一次，切到当时最新的就绪任务。

所以换人分两步：SysTick(System Tick: 内核节拍定时器)（或 `os_delay`）负责拍板，`PendSV_Handler` 负责执行。

## R4–R11(Register 4–11: 被调函数要保存的通用寄存器)

R4 到 R11(Register 4–11: 被调函数要保存的通用寄存器) 是 CPU(Central Processing Unit: 中央处理器) 里的通用寄存器。C 源码里写这些名字不会通过，编译器按 AAPCS(Procedure Call Standard for the ARM Architecture: ARM 过程调用标准) 自己分配。这 8 个是被调函数要保存的寄存器。

汇编可以直接点名。`STMDB R0!, {R4-R11}` 先把指针减 32 字节，再把 8 个寄存器写入这块内存。R4 在最低地址，R11 在最高地址（旧指针减 4）。

异常进入时，硬件已经把 R0–R3、R12、LR(Link Register: 链接寄存器，存放返回地址)、PC(Program Counter: 程序计数器)、xPSR(Program Status Register: 程序状态寄存器) 压进**当前正在用的栈**。任务跑在 PSP(Process Stack Pointer: 进程栈指针) 上时，这 8 个字进的是任务栈。`PendSV_Handler` 因此只再保存 R4–R11，然后改 PSP(Process Stack Pointer: 进程栈指针)。两边加起来才是完整现场。

## TCB(Task Control Block: 任务控制块)

TCB(Task Control Block: 任务控制块) 是一份任务档案。一个任务对内核来说不只是函数入口，还要有这份档案。本工程的档案是 `tcb_t`：

- `sp`：sp(stack pointer: 栈指针)，上次停下来时栈停在哪。这是第一项，PendSV(Pendable Service Call: 可挂起服务调用) 用 `[tcb]` 直接当栈指针
- `priority`：0 到 7，数值越大越优先。idle 固定是 0
- `ticks_remain`：当前时间片还剩几拍
- `delay_ticks`：还要睡几拍。非 0 表示它不在就绪链表里
- `next` / `prev`：同一优先级的循环双向链表
- `delay_next`：延时单向链表
- `stack_base`：这块栈的最低地址，用来数还剩多少填充字

没有单独的状态字段。在就绪链表上就是就绪，在延时链表上就是在等 tick。没有任务名，也没有删除任务。不要把 FreeRTOS 的 `TCB_t` 里那几十个成员当成这里已经有了。

## Idle

Idle 是空闲任务。`os_init` 自动创建，优先级 0。

调度器的规矩是：系统在跑，就必须至少有一个任务可以执行。用户任务都在 `os_delay` 里时，CPU(Central Processing Unit: 中央处理器) 不能没有下一条指令。`idle_task` 就是这个兜底：循环里执行 `wfi`。wfi(Wait For Interrupt: 等待中断) 让核睡到下一个中断，用来省电。

FreeRTOS 的 idle 还会在空闲时释放已删除任务的内存。本内核没有删除任务，idle 不做回收。

## tick 到了是不是立刻切换

是中断，但是两步。

SysTick(System Tick: 内核节拍定时器) 计数到 0，进入 `SysTick_Handler`。这个函数不做换栈。它做三件事：

1. `SysTime_OnTick`：单调毫秒加一，秒内毫秒满 1000 就进位。OLED(Organic Light-Emitting Diode: 有机发光显示屏) 时钟和 BMP280 忙等读的是这套数。
2. 延时链表每个任务减 1，减到 0 的放回就绪链表，并重装时间片。
3. 看当前时间片是否用尽，或就绪位图里是否有更高优先级。需要换人时只挂起 PendSV(Pendable Service Call: 可挂起服务调用)。

真正保存 R4–R11、调用 `schedule_next`、改 PSP(Process Stack Pointer: 进程栈指针) 的是随后的 `PendSV_Handler`。这样换栈这点开销不会插在串口或 DMA(Direct Memory Access: 直接存储器访问) 中断中间。

调度器还没启动时（`os_running == 0`），SysTick(System Tick: 内核节拍定时器) 只做第 1 步。否则 BMP280 初始化还在 `main` 里忙等，链表却可能被改掉。`os_running` 在 `SVC_Handler` 把 PSP(Process Stack Pointer: 进程栈指针) 装好之后才写成 1。SVC(Supervisor Call: 监督调用) 是这条启动路径用的异常。

同优先级且这一级只有当前任务时，时间片用尽只重装，不挂 PendSV(Pendable Service Call: 可挂起服务调用)。不重装的话，每毫秒都会空切一次。

## 上下文切换是否必须汇编

这条抢占路径必须汇编。

C 编译器按调用约定占用 R4–R11，也写不出“把这 8 个寄存器存进当前任务栈、把 PSP(Process Stack Pointer: 进程栈指针) 指到下一个任务、用 `0xFFFFFFFD` 从异常返回”。`0xFFFFFFFD` 是异常返回值：回到线程模式，并且用 PSP(Process Stack Pointer: 进程栈指针)。它放在 Handler 自己的 LR(Link Register: 链接寄存器，存放返回地址) 里，不是任务栈帧里那个 LR(Link Register: 链接寄存器，存放返回地址)。

`PendSV_Handler` 要 `BL schedule_next`。BL(Branch with Link: 带链接跳转，把返回地址写入链接寄存器) 会改写 LR(Link Register: 链接寄存器，存放返回地址)，把异常返回值冲掉，所以调用前先把 LR(Link Register: 链接寄存器，存放返回地址) 压进主栈 MSP(Main Stack Pointer: 主栈指针)。任务现场仍在 PSP(Process Stack Pointer: 进程栈指针) 上。

`setjmp` / `longjmp` 只能保存一部分环境，不够做这种带异常返回的抢占切换。本工程不用它们。

协作式调度可以在任务自己调用某个函数时才换人。本工程是 SysTick(System Tick: 内核节拍定时器) 到点就抢占，所以核心切换在 `port.c` 的裸函数里。

## 栈写穿看起来像非法返回

任务栈在 `.bss` 里一块挨着一块，向低地址生长。.bss(Block Started by Symbol: 未初始化全局数据段) 里的栈在上电后被涂成填充字。高优先级任务的栈底紧贴着下一块任务的栈顶。`printf` / `vsnprintf` 的调用栈若越过本任务的 `stack_base`，写掉的是下一块任务上次保存的异常帧。那块任务再被换回来时，硬件弹出的 PC(Program Counter: 程序计数器) 不是代码地址。CFSR(Configurable Fault Status Register: 可配置故障状态寄存器) 里的 UFSR(Usage Fault Status Register: 用法故障状态寄存器).INVPC(Invalid PC: 异常返回时装入了非法程序计数器)（`0x00040000`）因此被置位。UsageFault(Usage Fault: 用法故障) 默认没开，HFSR(HardFault Status Register: 硬故障状态寄存器) bit30 把它收成 HardFault(Hard Fault: 硬故障)。Handler 的 LR(Link Register: 链接寄存器，存放返回地址) 若是 `0xFFFFFFFD`，这 8 个字在 PSP(Process Stack Pointer: 进程栈指针) 上。

2026-11-24：状态任务的 `LOG_D` 水位约 1028 字节，1024 字节栈写进显示任务。`OS_TASK_STACK_WORDS` 现为 384（1536 字节）。不要为了省 RAM(Random Access Memory: 随机存取存储器) 改回 256。
