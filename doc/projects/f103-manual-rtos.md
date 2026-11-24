# f103-manual-rtos

STM32F103C8T6 上的手写调度工程。外设寄存器从 [`f103-manual-reg`](f103-manual-reg.md) 复制，引脚和日志文案与那条 demo 相同。差别是主循环拆成任务，由 SysTick 拍板、PendSV 换栈。

它不是第三条必须与 manual-reg / cmsis-hal 同步的功能路线。那两条 demo 改行为时，不要默认改这里。`./scripts/build.sh both` 也不包含本工程。

原理六问：[手写调度：上下文切换](../learn/manual-rtos-context-switch.md)。单步演示：[调度器介绍页](../learn/f103-manual-rtos-scheduler.html)。

## 构建

```bash
./scripts/build.sh f103-manual-rtos          # 首次或 clean 之后：configure + build
./scripts/build.sh f103-manual-rtos build    # 已有 build/ 时只编译
./scripts/build.sh f103-manual-rtos flash    # 须先 build。会换掉片上当前固件
```

产物：`projects/f103-manual-rtos/build/f103-manual-rtos.elf`。

不链接 CMSIS / HAL，没有 `SystemCoreClock`，也不把时基改到 TIM2。TIM2 是 PA1 风扇 PWM。

## 硬件

与 manual-reg 同一块板：

| 功能 | 引脚 |
|------|------|
| 板载 LED | PC13，Backup 域，先 PWREN + DBP |
| 扩展 LED | PB12 |
| 按键 | PB13，EXTI15_10，ISR 只置边沿标志 |
| 串口 | USART1，PA9 TX / PA10 RX，1500000 8N1，DMA + IDLE |
| 旋钮 | ADC1 PA0，DMA 循环 |
| 风扇 | TIM2 CH2，PA1，24 kHz |
| 气压 | SPI1 BMP280，CS = PA3。失败时 OLED 画 `0.00C` |
| 六轴 | LSM6 驱动链进 ELF，任务不调用。CS = PA8 |
| 屏 | I2C1 SH1106，PB6/PB7，写地址 `0x78` |

供电和共地见 [power-and-common-ground.md](../hardware/power-and-common-ground.md)。

## 任务

`os_init` 创建优先级 0 的 idle（`wfi`）。数值越大越优先。

| 任务 | 优先级 | 周期 | 做什么 |
|------|--------|------|--------|
| `task_key` | 4 | 5 ms | 取 EXTI 边沿，日志仍是 `PB13 KEY irq high/low` |
| `task_usart` | 4 | 1 ms | `USART1_ProcessRx` 回显 |
| `task_fan` | 3 | 1 ms | 读旋钮，`Fan_SetDuty`。原主循环是每次都写，节拍下限是 1 ms |
| `task_display` | 2 | 50 ms | 四段钟和缓存温度 |
| `task_status` | 1 | 1000 ms | 翻转 PC13 / PB12，读 BMP280，打原来的 `LOG_D` |

温度由 status 单写、display 单读。旋钮原始值和占空比由 fan 单写、status 单读。内核没有队列和信号量。

按键任务和状态任务都会 `LOG_*`，回显也会 `USART1_Write`。`USART1_Lock` 按当前 TCB 记深度：占不到就 `os_delay(1)`，等 DMA 完成时中断保持打开。同一任务里 `log_write` → `printf` → `USART1_Write` 可以重入。调度器启动前这把锁不做任何事，初始化日志仍是原来的忙等。

`os_stack_free_words` 从栈底数 `0xA5A5A5A5` 填充还剩多少字。不写进周期日志。

## 调度器相对教学原稿的修正

原稿可以直接对上 PendSV / SVC / 位图这套骨架，有几处不改会在 F103 上跑偏：

- 时间片减到 0 后重装 `OS_TIME_SLICE_TICKS`。同优先级还有别的任务时，才把该优先级链表头拨到 `current->next`。这一级只剩自己时不挂 PendSV。
- 更高优先级就绪时直接选那条链表的头，不旋转被抢占的任务。
- `os_running` 在 `SVC_Handler` 装好 PSP 之后才置 1。在此之前 SysTick 只加毫秒，供 `SysTick_GetMs` 忙等。SVC 保持复位优先级，高于 SysTick。
- `BL schedule_next` 会冲掉 LR 里的异常返回值。PendSV 在调用前把 LR 压进 MSP。

每个任务栈 384 字（1536 字节），池子 8 字节对齐。状态任务的 `LOG_D` 会进 `printf`，1024 字节栈的水位约 1028 字节，会写进下一块任务栈。8 块栈在 `.bss` 里。一次编译后 `_ebss = 0x20004110`，`_estack = 0x20005000`，中间约 3.7 KB 给堆和中断主栈。

和 FreeRTOS 比，这里没有队列、信号量、事件组、任务通知、软件定时器，也没有按到期时间排序的延时表。延时链表每拍线性走一遍，任务数最多 8。

## 启动顺序

1. `main` 最先 `NVIC_SetPriorityGroup4`：`SCB_AIRCR` 键值 `0x05FA`，`PRIGROUP = 3`（4 位全抢占，无子优先级）。按键和串口随后才写 NVIC IP。
2. 外设初始化与 manual-reg 相同。`SystemInit` 已在进 `main` 前把 HSE×9 配到 72 MHz。
3. 串口起来后读 `RCC_CFGR` 的 SWS。不是 PLL 就 `LOG_E`：HSE 失败时系统停在 HSI 8 MHz，重装值 71999 会变成约 9 ms。不新增 `SystemCoreClock`。
4. `SysTick_Init` 打开 1 ms 中断。BMP280 初始化和 OLED 清零都发生在 `os_start` 之前。
5. `os_start` 把 SysTick 和 PendSV 的 SHPR 字节写成 `0xF0`（实现的高 4 位为 15，即最低），再 `svc #0`。不调用 CMSIS `NVIC_SetPriority(..., 0xFF)`，那个参数还会被左移。

startup 里的 `PendSV_Handler`、`SVC_Handler`、`SysTick_Handler` 保持 WEAK。强符号各一份：前两个在 `port.c`，SysTick 在 `os.c`。

## 检查清单

| 项 | 怎么确认 | 不对时 |
|----|----------|--------|
| 时钟源是 PLL | 启动日志没有 `SYSCLK is not PLL` | 节拍约 9 ms，延时和四段钟都变慢 |
| 分组是 4 位抢占 | `main` 里先调用 `NVIC_SetPriorityGroup4` | PendSV 设成最低仍可能无法按抢占优先级排队 |
| 栈 8 字节对齐 | `stack_pool` 的 `aligned(8)` | `LDMIA {R4-R11}` 进 UsageFault，再进 HardFault |
| 三个 Handler 只有一份强符号 | `nm` 里各一个 `T` | 链接重复定义，或仍落到 startup 的 `Default_Handler` |
| idle 已创建 | `os_init` 里 `task_create(idle_task, 0)` | `os_start` 看见就绪位图为 0，直接返回 |
| SysTick / PendSV 为 `0xF0` | `os_start` 写 `SCB_SHPR3` 字节 2 和 3 | 切换打断外设中断 |

## 故障对照

| 现象 | 先查 |
|------|------|
| HardFault，CFSR `INVPC`（`0x00040000`） | 打日志的任务栈是否写穿进下一块。再看栈是否 8 字节对齐、任务函数是否 return |
| `PendSV_Handler` 或 `SysTick_Handler` 重复定义 | 是不是又写了一份强符号。startup 里的 WEAK 不要改成强符号 |
| 任务一个都不跑 | `os_init` 是否建了 idle；`ready_bitmap` 是否为 0 |
| 节拍不对、延时变长 | HSE 是否失败，SWS 是否仍为 PLL |
| `os_delay` 不切换 | 是否挂了 PendSV；改链表时是否在临界区里 |
| 日志或回显互相踩 | `USART1_Lock` 是否还包住 `log_write` 和 `USART1_Write` |
| 想用 `HAL_Delay` 或把时基改到 TIM2 | 本工程不链接 HAL。TIM2 是风扇 PWM，不能让给 HAL 时基 |

## 2026-11-24 停核

调度已经起来：`os_running == 1`，PC 在 `Default_Handler`（与 `HardFault_Handler` 同一地址），xPSR 的 IPSR 为 3。LR 为 `0xFFFFFFFD`，异常帧在 PSP 上，不在 MSP。CFSR 为 `0x00040000`（UFSR.INVPC），HFSR 的 bit30 为 FORCED。`current_tcb` 指向显示任务。PSP 上那帧的 PC 是 `1`。状态任务栈底再往下的一个字是 `USART1_Write` 里的返回地址。`LOG_D` 的 `printf` / `vsnprintf` 用了约 1028 字节，1024 字节的栈写进了紧挨着的显示任务栈。

`OS_TASK_STACK_WORDS` 改为 384 后再烧。CFSR 与 HFSR 为 0，PC 在显示任务的 `I2C1_WriteDma`，状态任务在 `os_delay`，两块栈底仍是 `0xA5A5A5A5`。`_ebss = 0x20004110`。

同一轮串口仍是 COM9、1500000、`--reset` 抓 8 秒、`bytes=0`。停核时 USART1 `BRR = 0x30`，`CR1 = 0x201C`（UE、TE、RE），`SR = 0x01D2`（含 TC、TXE），`GPIOA_CRH` 里 PA9 半字节为 `0xB`，`RCC_CFGR = 0x001D840A`（SWS 为 PLL）。外设布局与 manual-reg 相同。停核步骤见 [probe-rs.md](../probe-rs.md)。

板上确认：约每秒一行 `LOG_D`（LED / KEY / knob / clock），OLED 四段钟在走，按键边沿仍是原来的 `PB13 KEY irq` 文案。
