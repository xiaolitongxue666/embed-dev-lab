# 问题与处理

已经碰到过的失败和对应处理。

| 现象 | 处理 |
|------|------|
| HSE 失败 | 落到 HSI 8 MHz。UART、I2C、SysTick 都会变慢或错。启动链是 `g_pfnVectors` → `Reset_Handler` → `SystemInit`（HSE×PLL→72 MHz）→ `main`，`-nostartfiles` |
| 未上电的 MCU | `JtagGetIdcodeError`。`probe-rs list` 为空表示 ST-Link USB 不在 |
| 想看启动日志却错过 | 读脚本打开串口后会清空缓冲。用 `./scripts/serial-ch341-read.sh --reset`，复位发生在清空之后 |
| 串口 `bytes=0`，COM 能打开 | 不是波特率问题。波特不对通常是乱码。2026-11-24：COM9 上 manual-reg / cmsis-hal 烧录成功仍是 0。同日 `f103-manual-rtos` 栈修好后仍是 0，但 USART1 `BRR=0x30`、`CR1=0x201C`、`SR` 含 TC/TXE、PA9 为 `0xB`、SWS 为 PLL。不要改波特，也不要改 manual-reg 接线 |
| `probe-rs list` 空，设备是 `VID_0000&PID_0002`，描述符请求失败 | `pnputil /restart-device` 那个实例。不要对 `0000:0002` 装 WinUSB。2026-11-24 重启后变为 `STLink V2 -- 0483:3748`。usbipd 关于 USBPcap 的警告与这次无关 |
| `Failed to open probe` / USB error 5 | ST-Link 已被另一次 probe-rs 或 gdb 占用。等 `flash` 结束，或结束 `probe-rs.exe` 与 `arm-none-eabi-gdb.exe`。gdb `-batch` 里的 `while` 会停在交互提示上并占住探针 |
| `f103-manual-rtos` HardFault，CFSR `0x00040000` | 状态任务 `printf` 写穿 1024 字节栈，显示任务弹出的 PC 为 `1`。栈改为 384 字。LR=`0xFFFFFFFD` 时帧在 PSP |
| SPI 读到 `0x00` | MISO 被拉死或 CS 没有选中该器件。`TransferByte` 超时返回 `0xFF` |
| BMP280 片选无效 | PA4 孔是坏的。CSB 接 PA3。SDO 接 PA6，不要接 GND |
| 这块台架经常没有 BMP280 温度 | 初始化失败时 OLED 画 `0.00C` |
| OLED 时间停住 | 面板单独供电，MCU 没写成功就留着旧显存。cmsis-hal 的 SPI 与日志若用 `HAL_MAX_DELAY`，主循环进不了 50 ms 刷屏。现为 `spi.c` 每字节 10 ms、`log.c` 每行 20 ms，到时返回 |
| OLED 刷新过频 | 整屏刷新约 20 ms 以上，不要每 10 ms 刷新 |

未经用户确认，不要用全片擦除来「修」烧录失败。
