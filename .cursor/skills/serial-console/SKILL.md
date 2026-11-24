---
name: serial-console
description: 读取或切换 CH341 串口时使用。serial-ch341-switch、serial-ch341-read、WSL picocom。波特率与 COM 口以当前固件和实际枚举为准。
---

# 串口控制

烧录用 ST-Link，不占用这条 USB 串口。看日志才切换 CH341。接线确认见 [board-power](../board-power/SKILL.md)。

| 项 | 值 |
|----|-----|
| MCU | USART1，PA9=TX，PA10=RX |
| USB-TTL | CH341 RX←PA9，TX→PA10，GND 直接到面包板 GND |
| VID:PID | `1a86:5523` |
| 帧 | 8N1，无流控 |
| 波特 | 以当前固件为准。demo 常见 1500000，改过就必须改参数 |

不要写死 `COM3` 之类的端口名。不要在 Windows 与 WSL 两端同时打开同一 CH341。Agent 不要去操作 SecureCRT 或其他串口 GUI。

## 看当前宿主

```bash
./scripts/serial-ch341-switch.sh status
```

`to-win` / `to-wsl` 需要管理员（usbipd-win）。可在 Windows Git Bash 或 WSL 里运行切换脚本。

```bash
./scripts/serial-ch341-switch.sh to-win
./scripts/serial-ch341-switch.sh to-wsl
./scripts/serial-ch341-switch.sh to-wsl --distro archlinux
```

`to-win --baud 115200` 只改提示文案里的波特率，不改固件。

## Windows：Agent 读日志

在 Git Bash：

```bash
./scripts/serial-ch341-read.sh
./scripts/serial-ch341-read.sh --baud 115200
./scripts/serial-ch341-read.sh --list
./scripts/serial-ch341-read.sh --port COM5 --no-switch
./scripts/serial-ch341-read.sh --send PING --seconds 3
EMBED_SERIAL_BAUD=921600 ./scripts/serial-ch341-read.sh
```

- 默认会 `to-win`，再按 VID:PID 找 COM，再按波特抓取
- `--baud` 或 `EMBED_SERIAL_BAUD`：与固件一致。默认 1500000
- `--port` 仅在 `--list` 确认那个 COM 是 CH341 之后使用
- `--seconds` 默认 5
- `--send` 打开后先写再读。文本，或 `hex:50494E47`
- `--no-switch` 不调用 `to-win`
- 依赖 pyserial。脚本优先 `uv run --with pyserial`。本机代理为 `http://127.0.0.1:7890`
- 要抓启动行：先让读脚本打开串口，再复位板子

WSL 里不要用 `serial-ch341-read.sh`。

## WSL：picocom

```bash
./scripts/serial-ch341-switch.sh to-wsl
ls /dev/ttyUSB*
picocom -b 1500000 /dev/ttyUSB0
```

波特与固件不一致时改 `-b`。节点不是 `ttyUSB0` 时用 `ls` 看到的那个。退出：Ctrl+A 然后 Ctrl+Q。查看配置：Ctrl+A 然后 Ctrl+V。

## 失败分支

| 现象 | 处理 |
|------|------|
| 无 COM / 无 tty | `status`，再 `to-win` 或 `to-wsl` |
| 乱码 | 对照工程 `usart.c` 或 HAL 初始化里的波特，然后 `--baud` 或 picocom `-b`。不要猜 |
| 打不开口 | 已有程序占用。关掉 GUI 或换宿主。不要两端一起占 |
| WSL Permission denied | 用户加入 `dialout`，或本次用 sudo |
| 有 LED 无日志 | 波特、RX←PA9、GND、宿主是否切到你正在读的那一侧 |
| HSE 失败导致波特全错 | 时钟掉到 HSI 8 MHz。先查时钟，不要只改串口参数 |

规则全文：[`.agent/rules/serial.md`](../../rules/serial.md)。脚本说明：[`doc/scripts-reference.md`](../../../doc/scripts-reference.md)。
