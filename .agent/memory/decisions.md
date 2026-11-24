# 技术决策

已落地的选择。台架是否跑通见 [`progress.md`](progress.md)。

- 构建：`./scripts/build.sh <module> build|flash`。`flash` 不编译。`clean` 之后先 `build`（默认 `all`，含 configure）。chip `STM32F103C8Tx`。烧录 `--binary-format elf`。
- 双路线对外行为对齐，除非用户明确只改一边。
- 两条路线都用软件定时：OLED 50 ms，LED 1 s。OLED 中央四段 `00:00:00:00`。`LOG_*` 为 `[HH:MM:SS][I]` 加 ANSI。
- 手写寄存器串口：`printf` → `syscalls` → `USART1_Write`，在 `g_usart1_tx_busy` 上阻塞。DMA1 CH4 TX、CH5 RX + `IDLEIE`。HTIE=0，只开 TCIE。RX 帧处理函数是 `USART1_EchoFrame`。cmsis-hal 不用这条 printf 路径。
- PC13 属 Backup 域：写 `GPIOC_CRH` 前必须 `PWREN` + `DBP`。
- I2C1：PB6/PB7 `AF_OD`，400 kHz，`CCR=30`，`TRISE=11`。SH1106 写地址用原始 `0x78`，不要再 `<<1`。页数据 `I2C1_WriteDma`，DMA1 CH6，128 字节。整屏刷新约 20 ms 以上，不要每 10 ms 刷新 OLED。
- SH1106：`DrawClock(h,m,s,centi)` 在 8×16 画 `00:00:00:00`（11 个字形，x=20，y=24）。两倍宽的 `HH:MM:SS` 放不进 128 列。温度仍在 y=48。刷新 50 ms（`TIMER_EVT_OLED_CLOCK`）。温度缓存，BMP280 在 1 s 槽读取。`Init` 之后：manual-reg `SetMs(0)`；cmsis-hal `SysTime_Reset()`（不动 `uwTick`）。
- manual-reg SysTick：保持 `HCLK / 71999`，不要用 `HCLK/8` 且 `LOAD=9000`，也不要用 CMSIS 配这条时基。ISR：`s_systick_ms++`（`GetMs`），`s_sys_ms++` / `sec++`，然后 `TimerEvent_OnTick()`。`SysTime_Get` 在 **sec** 上重试（不是 ms），避免撕裂。关中断用 `mrs PRIMASK` + `cpsid`，再按原值恢复。
- `timer_event`：4 个槽。`period_ms==0` 跳过。`TakeFlag` 读清。OLED 50 ms 周期；LED/日志 1000 ms 周期。`main` 里没有 `delay(0x7FFF)` / `led_phase`。manual-reg：外设层 + `cpsid`。cmsis-hal：扁平 `src/` + CMSIS `__disable_irq`；`SysTick_Handler` 先 `HAL_IncTick` 再 `OnTick`。
- `LOG_*`：只显示到秒。E 红 / W 黄 / I 绿 / D 青。`LOG_COMPILE_LEVEL` 默认 DEBUG。`LOG_COLOR=1`。一行一次 `vsnprintf`、一次发送。1 s 的 DEBUG 是一行（LED/KEY/旋钮/时钟，温度可有可无）。不要在 ISR 里打日志。SecureCRT 需要 ANSI Color。manual-reg 一次 `printf`。cmsis-hal 一次 `HAL_UART_Transmit`（无 syscalls），发送超时 20 ms。
- cmsis-hal `SPI1_TransferByte` 每字节超时 10 ms，失败返回 `0xFF`。引脚、Mode0/Mode3 与片选不变。BMP280 缺席时不得挡住 50 ms OLED 刷新。
- cmsis-hal：先 `fetch-cmsis.sh`，再 `fetch-f103-cmsis-hal-deps.sh`。HAL Src 16 个 `.c`。工程内有 `src/log.c` 与 `src/timer_event.c`。`src/`、`startup/`、`linker/` 注释中文，CLI 英文。时钟在 `main` 的 `SystemClock_Config`。
- 未经确认不做全片擦除。
- 供电：蓝板 MicroUSB 给 MCU。ST-Link 3.3V 到面包板（方案 A，5V 闲置）。地用星型汇集。见 [`../rules/hardware.md`](../rules/hardware.md)。
