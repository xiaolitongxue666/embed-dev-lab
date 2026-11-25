# 项目进度

台架上已经确认过的状态。失败现象见 [`failures.md`](failures.md)。

- 2026-09-21：两条路线都已接上软件定时（OLED 50 ms / LED 1 s）、四段 OLED `00:00:00:00`、`LOG_*` `[HH:MM:SS][I]` + ANSI。
- 2026-09-21：两条路线烧录通过。当时 Windows 口是 COM8，波特 1500000。约每秒一行青色：`[00:00:01][D] LED on  KEY high  knob raw=…  clock 00:00:01:00`。启动 `[I]` 为绿色。OLED 四段时钟在走。
- 同一块台架上 BMP280 温度经常没有（初始化失败时画面为 `0.00C`）。
- LSM6DS3：2026-09-16 硬件还没上到这块台架。`main` 不调用它。
- 2026-11-24：两条路线都重新 `build` 并通过 probe-rs 烧录（ST-Link V2 `0483:3748`，未做全片擦除）。最后驻留的是 `f103-cmsis-hal`。CH341 在 Windows 上是 COM9（`1a86:5523`）。1500000 下多次抓取，包括打开端口后复位和发送 `PING`，都是 `bytes=0`。SWD 能烧录，串口没有边沿。
- 2026-11-24：`f103-manual-rtos` 已烧录（未做全片擦除）。第一次停核是 HardFault INVPC，原因是状态任务 `printf` 写穿 1024 字节栈。栈改为 384 字后 CFSR 为 0，显示任务在刷 SH1106，状态任务在 `os_delay`。COM9 复位后抓 8 秒仍是 `bytes=0`，USART 侧 TC 已置位。外设连接与 manual-reg 相同。
- 2026-11-24：调度器介绍页 `doc/learn/f103-manual-rtos-scheduler.html`。单步按 `os.c` / `port.c`：时间片 1，同级才轮转，抢占不拨链表头，`BL schedule_next` 前 `push {lr}`。演示把 usart 画成 2 拍、key 画成 4 拍，源码仍是 `os_delay(1)` 与 `os_delay(5)`。
- 2026-11-25：同一页在页头下增加深色点阵「调度总览」（中枢 / READY / RUNNING / DELAYED，贝塞尔虚线与光点）。右侧原流程图连线也流动。配色和单步脚本未改。约 850px 高时总览 190px、任务行 150px、底栏 104px，避免流程图被挤没。
