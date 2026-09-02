# 快速上手

从零开始在 **STM32F103C8T6 核心板** 上点亮 PC13 LED，并可看串口日志。命令与 CLI 输出为英文。

日常「改代码 → 编译 → 烧录」及**烧写脚本位置**见 [编写 → 编译 → 下载](workflow-write-build-flash.md#烧写脚本在哪里)。

## 前置

| 项目 | 说明 |
|------|------|
| OS | Windows（Git Bash）/ Linux / macOS |
| IDE | Cursor 或 VS Code（推荐） |
| 硬件 | ST-Link V2、F103 核心板、USB 线、杜邦线；可选 USB-TTL（串口） |

Windows 请安装 [Git for Windows](https://git-scm.com/download/win)，Cursor 终端默认 **Git Bash**。

---

## 步骤 1：克隆并进入仓库

```bash
git clone --recursive <repo-url>
cd embed-dev-lab
```

若已普通 clone、需拉取 CMSIS 子模块（**core + device-f1 + HAL driver**）：

```bash
./scripts/fetch-cmsis.sh
```

## 步骤 2：一键环境 + 编译

脚本默认使用 HTTP 代理 `http://127.0.0.1:7890`。无本地代理时：

```bash
./scripts/bootstrap.sh --no-proxy
```

有代理或可忽略：

```bash
./scripts/bootstrap.sh
```

等价于：安装工具 → 配置 PATH → 安装扩展 → 校验 → 编译 `f103-manual-reg` → 配置 clangd。**不含烧录。**

若工具已装好：

```bash
./scripts/bootstrap.sh --build-only --skip-extensions
```

## 步骤 3：ST-Link 驱动（Windows）

```bash
./scripts/install/stlink-winusb-windows.sh --check-only
probe-rs list
```

若 list 为空，见 [probe-rs.md — WinUSB](probe-rs.md#windows--winusb)。

Linux 配置 udev；macOS 跳过。

## 步骤 4：接线

**供电**（两套电源互相独立，见 [供电与共地](hardware/power-and-common-ground.md)）：

| 路径 | 说明 |
|------|------|
| 电脑 USB → 蓝板 MicroUSB | **MCU 独立供电**（必须） |
| 电脑 USB → ST-Link USB | 调试器供电；其 **3.3V / GND 只**拉到面包板（方案 A：5V 闲置），**禁止**接到蓝板电源脚 |

**SWD**（信号 + 共地；调试下载，**不是**串口，见 [swd-vs-usart.md](learn/swd-vs-usart.md)）：

| ST-Link / 地 | 接到 |
|--------------|------|
| SWDIO | 蓝板 PA13 |
| SWCLK | 蓝板 PA14 |
| GND | **面包板 GND 轨**（再从该轨短线到蓝板 SWD GND；CH341 GND 也直接进该轨） |

接外设时：ST-Link **3.3V / GND** → 面包板电源轨；当前传感器走 3.3V 轨。共地星型接法见 [供电与共地](hardware/power-and-common-ground.md)。

确保蓝板已由 MicroUSB 供电后再烧录。

## 步骤 5：编译 + 烧录

```bash
./scripts/build.sh f103-manual-reg build
./scripts/build.sh f103-manual-reg flash
```

或一键：

```bash
./scripts/build-flash.sh f103-manual-reg
```

或 Run Task → **Build and Flash F103**。

## 步骤 6：验证 LED

PC13 连接 LED 应约 **1 秒周期闪烁**（多数板子低电平点亮）。

若无闪烁：按板载 **RESET**；见 [f103-manual-reg 模块](projects/f103-manual-reg.md) 与 [probe-rs.md 排错](probe-rs.md#故障排查)。

## 步骤 7（可选）：串口日志

| 项 | 值 |
|----|-----|
| USB-TTL | **RX←PA9**，TX→PA10，**GND→面包板 GND 轨**（CH341 = USB↔TTL，见 [uart-ttl-rs232-rs485.md](learn/uart-ttl-rs232-rs485.md)） |
| 波特率 | **1500000** 8N1 |

应看到 `Stm32 manual reg demo start` 与周期 `LED on` / `LED off`（字符串指 GPIO 电平，非灯物理亮灭；低电平点亮板上 LED）。

若使用 **CH341**，且需在 Windows COM 与 WSL `/dev/ttyUSB0` 之间切换：

```bash
./scripts/serial-ch341-switch.sh to-win    # Windows 串口助手
./scripts/serial-ch341-switch.sh to-wsl    # WSL 终端读串口
./scripts/serial-ch341-switch.sh status
```

WSL 下用 **picocom**（波特率与固件一致；demo 默认 **1500000**）：

```bash
picocom -b 1500000 /dev/ttyUSB0
```

Windows 下 Agent 读串口（自动找 CH341 COM，勿写死端口）：

```bash
./scripts/serial-ch341-read.sh
./scripts/serial-ch341-read.sh --baud 115200   # 固件波特已改时
```

需 [usbipd-win](https://github.com/dorssel/usbipd-win) 与管理员权限。说明：[scripts-reference.md § serial-ch341](scripts-reference.md#serial-ch341-switchsh--ch341-串口宿主切换windows--wsl)。

---

## 可选：f103-cmsis-hal

CubeIDE 风格对照工程（HAL），行为对齐 manual-reg：

```bash
./scripts/fetch-cmsis.sh
./scripts/fetch-f103-cmsis-hal-deps.sh
./scripts/build.sh f103-cmsis-hal
./scripts/build.sh f103-cmsis-hal flash
```

详见 [f103-cmsis-hal](projects/f103-cmsis-hal.md) 与 [编写 → 编译 → 下载](workflow-write-build-flash.md)。

## 可选：IDE 调试

1. F5 → **F103 Probe-rs Debug**（manual-reg）或 **F103 CMSIS-HAL Probe-rs Debug**
2. 自动编译、烧录、断点

详见 [ide-debug.md](ide-debug.md)。

---

## 下一步

- [编写 → 编译 → 下载](workflow-write-build-flash.md)
- [F103 硬件外设与接线](hardware/stm32f103-peripherals.md) — 1.3″ SH1106 I2C、SPI LSM6DS3 接线与采购
- [供电、共地与 SWD](hardware/power-and-common-ground.md) — 蓝板 MicroUSB、ST-Link 3.3V→面包板、GND 星型汇集
- [LSM6DS3 参考](reference/lsm6ds3/README.md) — DocID026899 中文精选
- [probe-rs 详细说明](probe-rs.md)
- [脚本完整参考](scripts-reference.md)
- [f103-manual-reg 源码说明](projects/f103-manual-reg.md)

维护速查：[PROJECT_MEMORY.md](../PROJECT_MEMORY.md)
