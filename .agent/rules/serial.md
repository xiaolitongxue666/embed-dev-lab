# CH341 串口：宿主切换与读日志

共用正文。Cursor 注入同一约束：[`.cursor/rules/serial-ch341.mdc`](../../.cursor/rules/serial-ch341.mdc)。操作步骤：[`.agent/skills/serial-console/SKILL.md`](../skills/serial-console/SKILL.md)。

## 硬件与可变参数

| 项 | 说明 |
|----|------|
| 外设 | USART1，PA9=TX，PA10=RX |
| USB-TTL | CH341 RX←PA9，TX→PA10，GND；VID:PID `1a86:5523` |
| 帧格式 | 8N1 无流控；波特率以当前固件为准（demo 常见 1500000，会改） |
| COM / tty | 端口名会变（如 COM3→COM5；WSL 多为 `/dev/ttyUSB0`，勿写死） |
| 与 SWD | 独立：烧录用 ST-Link；看日志才需串口 |
| 共地 | CH341 GND 直接接面包板 GND 轨（勿串联经蓝板）；见 [`hardware.md`](hardware.md) |

改波特后：对照工程 `usart.c` / HAL 初始化，用 `--baud` 或 `EMBED_SERIAL_BAUD`，不要假定永远是 1500000。

## 宿主切换（须管理员）

```bash
./scripts/serial-ch341-switch.sh status
./scripts/serial-ch341-switch.sh to-wsl
./scripts/serial-ch341-switch.sh to-win
./scripts/serial-ch341-switch.sh to-win --baud 115200
```

`to-win --baud` 只影响提示文案中的波特率。勿在 Windows COM 与 WSL 两端同时占用同一 CH341。

## Agent 读串口

### Windows

```bash
./scripts/serial-ch341-read.sh
./scripts/serial-ch341-read.sh --baud 115200
./scripts/serial-ch341-read.sh --port COM5 --no-switch
./scripts/serial-ch341-read.sh --list
./scripts/serial-ch341-read.sh --send PING --seconds 3
EMBED_SERIAL_BAUD=921600 ./scripts/serial-ch341-read.sh
```

- COM：默认按 VID:PID 自动检测；仅在 `--list` 确认后才可 `--port` 强制
- 波特：`--baud` / `EMBED_SERIAL_BAUD`；默认 1500000
- 依赖：`pyserial`（脚本优先 `uv run --with pyserial`）
- 人工 GUI（SecureCRT 等）给用户用；Agent 不要驱动 GUI

### WSL（picocom）

```bash
./scripts/serial-ch341-switch.sh to-wsl
picocom -b <波特率与固件一致> /dev/ttyUSB0
```

退出 Ctrl+A Ctrl+Q；确认配置 Ctrl+A Ctrl+V。设备节点若非 `ttyUSB0`，先 `ls /dev/ttyUSB*`。

## 排错

| 现象 | 处理 |
|------|------|
| 无 COM / 无 tty | `status` → `to-win` 或 `to-wsl` |
| 乱码 | 波特与固件不一致；查源码后 `--baud` |
| 打不开口 | SecureCRT 等已占用；关掉或换宿主 |
| Permission denied (WSL) | `dialout` 或 sudo |

详解：[`doc/scripts-reference.md`](../../doc/scripts-reference.md)。流程：[`doc/workflow-write-build-flash.md`](../../doc/workflow-write-build-flash.md)。
