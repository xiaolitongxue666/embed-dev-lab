# f103-manual-rtos

共用正文。Cursor 在 `projects/f103-manual-rtos/**` 注入同一约束：[`.cursor/rules/f103-manual-rtos.mdc`](../../.cursor/rules/f103-manual-rtos.mdc)。

手写 SysTick + PendSV 调度。外设寄存器复制自 `f103-manual-reg`，不参与 manual-reg 与 cmsis-hal 的行为同步。`./scripts/build.sh both` 不编译本工程。

- 不 `#include` / 链接官方 CMSIS Device 头或 HAL
- SysTick 的强符号只在 `src/os/os.c`。`PendSV_Handler` 与 `SVC_Handler` 只在 `src/os/port.c`。startup 里的 WEAK 不要改成强符号，也不要再写第二份
- 不把 TIM2 改成系统时基。TIM2 CH2 = PA1，是 JY003 风扇 PWM
- 不用 `HAL_Delay`。任务里等节拍用 `os_delay`。BMP280 在调度器启动前仍用 `SysTick_GetMs` 忙等
- 串口仍是 `LOG_*` → `printf` + `syscalls.c` → `USART1_Write`。多任务发送走 `USART1_Lock`，等 DMA 时不要关中断
- `OS_TASK_STACK_WORDS` 保持 384。状态任务的 `printf` 在 256 字栈上会写进下一块任务，表现为 HardFault INVPC。不要改回 256
- 中断里不调用 `os_delay` / `task_create`
- 时钟在 `SystemInit` 升至 72 MHz。SysTick 重装值 71999。没有 `SystemCoreClock`
- `main` 在任何 `NVIC_IRQ_SetPriority` 之前调用 `NVIC_SetPriorityGroup4`
- 构建：`./scripts/build.sh f103-manual-rtos`；文档：[`doc/projects/f103-manual-rtos.md`](../../doc/projects/f103-manual-rtos.md)；原理：[`doc/learn/manual-rtos-context-switch.md`](../../doc/learn/manual-rtos-context-switch.md)
