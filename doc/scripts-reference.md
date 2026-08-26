# 脚本参考

本文列出 `scripts/` 下各脚本的用途、典型命令及 **自动化链路**。脚本日志前缀为 `[embed-dev-lab]`，输出为英文。

日常流程见 [编写 → 编译 → 下载](workflow-write-build-flash.md)。

## 烧写脚本在哪里

| 文件 | 作用 |
|------|------|
| [`scripts/build.sh`](../scripts/build.sh) | **真正烧录**：`flash`（probe-rs ELF）/ `flash-openocd`（OpenOCD HEX） |
| [`scripts/build-flash.sh`](../scripts/build-flash.sh) | 一键：`build` → `flash`（不 configure） |
| [`.vscode/tasks.json`](../.vscode/tasks.json) | Task「Build and Flash F103」等，调用 `build.sh` |
| [`.vscode/launch.json`](../.vscode/launch.json) | F5 时 probe-rs-debugger 烧录 |

`bootstrap.sh` **不含**烧录。流程细节：[workflow-write-build-flash.md § 烧写脚本在哪里](workflow-write-build-flash.md#烧写脚本在哪里)。

## 自动化总览

```mermaid
flowchart TB
    subgraph bootstrap_flow [bootstrap.sh]
        B1[install-tools.sh] --> B2[setup-path 内嵌刷新]
        B2 --> B3[install-extensions.sh 可选]
        B3 --> B4[env-check.sh]
        B4 --> B5["build.sh f103-manual-reg"]
        B5 --> B6[setup-clangd.sh]
    end

    subgraph cli_flash [CLI 烧录]
        C1["build.sh build"] --> C2["build.sh flash"]
    end

    subgraph ide_flow [IDE F5 调试]
        I1[Task Build F103] --> I2[build.sh f103-manual-reg]
        I2 --> I3[probe-rs-debugger flash+debug]
    end
```

`env-check`：完整 bootstrap（装扩展）时检查扩展；`--skip-extensions` 或 `--build-only` 时用 `--tools-only`。

---

## 入口脚本

### bootstrap.sh — 一键环境 + 编译

**不含烧录。**

```bash
./scripts/bootstrap.sh                    # 安装 + 扩展 + 编译 f103-manual-reg + clangd
./scripts/bootstrap.sh --build-only       # 跳过安装，仅 PATH + 编译
./scripts/bootstrap.sh --install-only     # 仅安装，不编译
./scripts/bootstrap.sh --skip-extensions  # 不装 IDE 扩展
./scripts/bootstrap.sh --module other     # 指定模块（默认 f103-manual-reg）
./scripts/bootstrap.sh --no-proxy         # 禁用默认代理 7890
```

步骤顺序：安装工具 → 刷新 PATH →（扩展）→ env-check → build → setup-clangd。

---

### build.sh — 模块构建与烧录

```bash
./scripts/build.sh <module> [action]
```

| action | 行为 |
|--------|------|
| `all`（默认） | configure + build + sync compile_commands + clangd |
| `configure` | 仅 CMake configure |
| `build` | 仅 ninja build（**不** configure；`build/` 须已存在） |
| `flash` | probe-rs download + reset（**不编译**） |
| `flash-openocd` | OpenOCD 烧录 hex |
| `clean` | 删除 build/ 与根 compile_commands.json |

**`clean` 之后**须先 `./scripts/build.sh <module>`（`all`），不要只用 `build` 或 `build-flash.sh`。

`f103-cmsis-hal` 首次 configure 前须：`./scripts/fetch-f103-cmsis-hal-deps.sh`。

示例：

```bash
./scripts/build.sh f103-manual-reg
./scripts/build.sh f103-manual-reg build
./scripts/build.sh f103-manual-reg flash
./scripts/build.sh f103-manual-reg build && ./scripts/build.sh f103-manual-reg flash
./scripts/build-flash.sh f103-manual-reg   # 同上；编译失败时暂停
./scripts/fetch-f103-cmsis-hal-deps.sh && ./scripts/build.sh f103-cmsis-hal
./scripts/build-flash.sh f103-cmsis-hal
```

---

### build-flash.sh — 一键编译并烧录

依次执行 `build.sh build` → `build.sh flash`（**不** configure）。`build/` 不存在时会失败并提示先跑 `build.sh <module>`。编译失败时 **ninja/gcc 报错原样输出**，并在交互终端 **暂停等待 Enter**。

```bash
./scripts/build-flash.sh              # 默认 f103-manual-reg
./scripts/build-flash.sh f103-manual-reg
./scripts/build-flash.sh f103-cmsis-hal
```

| 步骤 | 行为 |
|------|------|
| `build/` 缺失 | 明确错误，指向 `./scripts/build.sh <module>` |
| build 失败 | 打印编译错误，`Press Enter to exit...`，exit 1 |
| build 成功 | probe-rs download + reset |

---

### serial-ch341-switch.sh — CH341 串口宿主切换（Windows ↔ WSL）

基于 **usbipd-win**，把 CH341 USB-UART（VID:PID `1a86:5523`）在 Windows COM 口与 WSL `/dev/ttyUSB0` 之间切换。可在 **Windows Git Bash** 或 **WSL** 中运行；`bind` / `attach` / `detach` / `unbind` 通常需要 **管理员权限**。

前置：已安装 [usbipd-win](https://github.com/dorssel/usbipd-win)（`usbipd.exe`，勿与 Linux 的 `/usr/bin/usbipd` 混淆）。

```bash
./scripts/serial-ch341-switch.sh                 # 默认：status
./scripts/serial-ch341-switch.sh status
./scripts/serial-ch341-switch.sh to-wsl          # 附加到 WSL → /dev/ttyUSB0
./scripts/serial-ch341-switch.sh to-win          # 归还 Windows → COM 口
./scripts/serial-ch341-switch.sh to-wsl --distro archlinux   # 指定发行版
```

| 动作 | 结果 | 读串口 |
|------|------|--------|
| `to-win`（默认宿主侧） | Windows COM 口 | 串口助手 **1500000** 8N1 |
| `to-wsl` | WSL `/dev/ttyUSB0` | `picocom -b 1500000 /dev/ttyUSB0`（见下节） |

硬件接线不变：CH341 **RX←PA9**，TX→PA10，GND 共地。烧录仍用 ST-Link（与串口宿主无关）。

**端口名与波特率会变**：COM 编号随插拔变化；固件改波特后须传 `--baud` / `EMBED_SERIAL_BAUD`。Windows Agent 读串口用下一节脚本（自动认 CH341），勿写死 `COM5` / 假定永远 `1500000`。

#### serial-ch341-read.sh — Windows Agent 读串口

在 **Windows Git Bash** 下：可选先 `to-win`，按 VID:PID **自动找 CH341 COM**，按指定波特率抓取若干秒（依赖 pyserial；优先 `uv run --with pyserial`）。

```bash
./scripts/serial-ch341-read.sh                         # to-win + 自动 COM + 默认 1500000
./scripts/serial-ch341-read.sh --baud 115200 --seconds 8
./scripts/serial-ch341-read.sh --port COM5 --no-switch # 强制口（须先 --list 确认）
./scripts/serial-ch341-read.sh --list
EMBED_SERIAL_BAUD=921600 ./scripts/serial-ch341-read.sh
```

| 选项 / 环境变量 | 含义 |
|-----------------|------|
| `--baud` / `EMBED_SERIAL_BAUD` | 与**当前固件**一致（默认 1500000） |
| `--port` / `EMBED_SERIAL_PORT` | 强制 COMx；默认自动检测 `1a86:5523` |
| `--seconds` | 抓取时长（默认 5） |
| `--no-switch` | 不调用 `to-win` |
| `--list` | 列出 COM 并标记 CH341 |

WSL 侧仍用 picocom（见下节），不要用本脚本。

#### WSL 下用 picocom 读串口

本仓库 demo **默认**波特率为 **1500000**（以固件为准，可改）。先把 CH341 切到 WSL，再开终端：

```bash
./scripts/serial-ch341-switch.sh to-wsl   # 出现 /dev/ttyUSB0（需管理员 + usbipd-win）
ls -l /dev/ttyUSB0                        # 确认节点存在
# 若 Permission denied：sudo usermod -aG dialout "$USER" 后重新登录，或临时 sudo
sudo apt install picocom                  # Debian/Ubuntu；Arch: pacman -S picocom
```

**日常用法（默认已是 8 数据位 / 无校验 / 1 停止位 / 无流控）：**

```bash
picocom -b 1500000 /dev/ttyUSB0
```

**显式写出 8N1、无流控**（与 man 一致：校验用 `-y`，停止位用 `-p`）：

```bash
picocom -b 1500000 -d 8 -y n -p 1 -f n /dev/ttyUSB0
```

启动后应看到类似：

```text
port is        : /dev/ttyUSB0
baudrate is    : 1500000
parity is      : none
databits are   : 8
stopbits are   : 1
```

烧录后的 demo 会周期输出 `LED on` / `LED off`（含 `\r\n`）。用 **Ctrl+A** 再 **Ctrl+V** 可复核 `baudrate is: 1500000`。

| 按键（默认转义前缀 Ctrl+A） | 功能 |
|-----------------------------|------|
| Ctrl+A Ctrl+Q | 退出且**不**复位串口参数（Quit） |
| Ctrl+A Ctrl+X | 退出；默认会复位串口并拉低 DTR/RTS（Exit） |
| Ctrl+A Ctrl+V | 显示当前配置（确认波特率） |
| Ctrl+A Ctrl+C | 开/关本地回显（MCU 不回显时便于看自己键入） |
| Ctrl+A Ctrl+B | 运行时改波特率 |
| Ctrl+A Ctrl+S | 发送文件（可配合 `-s` 指定外部传输命令） |
| Ctrl+A Ctrl+H | 快捷键帮助 |

> 转义前缀默认是 **Ctrl+A**（`-e a`）。`-e x` 只是把转义改成 Ctrl+X，**不是**关闭转义；要禁用命令模式用 `-n` / `--no-escape`（此后只能关终端或发信号退出）。

**记录会话日志**（`--logfile` 与 `-g` 等价；已有文件则追加）：

```bash
picocom -b 1500000 --logfile /tmp/uart.log /dev/ttyUSB0
```

**只看原始流、不交互**（Ctrl+C 结束；一般仍建议先 `stty` 设波特率，或直接用 picocom）：

```bash
cat /dev/ttyUSB0
```

**波特率说明**：Linux `ch341` 驱动可协商约 **46 bps～3 Mbps** 的自定义速率；**1500000** 在 picocom 的 `HIGH_BAUD` 列表中，且本仓库在 WSL（usbipd attach → `/dev/ttyUSB0`）上已验证可收到 `LED on\r\n` / `LED off\r\n` 无乱码。若出现 `Cannot set baud rate` 或乱码：确认已 `to-wsl`、接线 RX←PA9、固件按 72 MHz 算 BRR；仍失败再排查内核/驱动版本。

调完切回 Windows 串口助手：

```bash
./scripts/serial-ch341-switch.sh to-win
```

参考：[picocom man page](https://manpages.debian.org/picocom/picocom.1.en.html)。

---

F103 chip：`STM32F103C8Tx`。ELF：`projects/<project>/build/<project>.elf`。

#### 构建产物与格式转换

工具链为 **`arm-none-eabi-gcc`**（裸机），非 `arm-linux-gnueabihf`。

| 文件 | 生成时机 | 用途 |
|------|----------|------|
| `<module>.elf` | Ninja 链接 | probe-rs `flash`、IDE F5 |
| `<module>.hex` | build 后 POST_BUILD（[`mcu-config.cmake`](../cmake/mcu-config.cmake) 内 `arm-none-eabi-objcopy -O ihex`） | `flash-openocd` |
| `<module>.bin` | **未生成** | — |

```text
build  → .elf + .hex（objcopy 自动）
flash  → probe-rs 烧录 .elf
flash-openocd → OpenOCD 烧录 .hex
```

CMake / 链接细节见 [f103-manual-reg 编译流程](learn/f103-module-build-flow.md)。

---

### install-tools.sh — 分 OS 安装

```bash
./scripts/install-tools.sh
./scripts/install-tools.sh --skip-extensions
./scripts/install-tools.sh --skip-env-check
```

调用 `scripts/install/{windows,linux,macos}.sh`，末尾可选 setup-path、扩展、env-check。

---

## 辅助脚本

| 脚本 | 用途 | 典型命令 |
|------|------|----------|
| `env-check.sh` | 校验 cmake/ninja/gcc/clangd/probe-rs、扩展、probe | `./scripts/env-check.sh` |
| `setup-path.sh` | 写入 User PATH + `~/.bashrc` embed-dev-lab 块 | `./scripts/setup-path.sh` |
| `setup-clangd.sh` | 同步 compile_commands、写 settings.local.json | `./scripts/setup-clangd.sh` |
| `install-extensions.sh` | 安装 `.vscode/extensions.json` 扩展 | `./scripts/install-extensions.sh` |
| `install/stlink-winusb-windows.sh` | Windows ST-Link WinUSB | `--check-only` / `--install` |

---

## env-check.sh 选项

```bash
./scripts/env-check.sh              # 工具 + 扩展 + probe（Windows）
./scripts/env-check.sh --tools-only # 仅 CLI 工具（bootstrap 内部用）
```

---

## lib/ 公共库

| 文件 | 作用 |
|------|------|
| `lib/common.sh` | 日志、`die`、工具检测 |
| `lib/os-detect.sh` | OS 判定、Git Bash 要求 |
| `lib/paths.sh` | 路径规范化、用户目录 |
| `lib/path-setup.sh` | PATH 写入 Windows/macOS/Linux |
| `lib/detect-toolchain.sh` | ARM GCC 探测 |
| `lib/proxy.sh` | 代理（默认 `http://127.0.0.1:7890`） |
| `lib/editor-detect.sh` | cursor/code CLI |

---

## 如何「自动」编译与烧录

| 目标 | 做法 |
|------|------|
| 自动装环境 + **编译** | `./scripts/bootstrap.sh` |
| CLI **编译 + 烧录** | `build.sh f103-manual-reg build && build.sh f103-manual-reg flash` |
| Task **编译 + 烧录** | Run Task → **Build and Flash F103** |
| IDE **编译 + 烧录 + 调试** | F5 → **F103 Probe-rs Debug** 或 **F103 CMSIS-HAL Probe-rs Debug** |
| CH341 **Windows ↔ WSL** | `./scripts/serial-ch341-switch.sh to-win` / `to-wsl`（需管理员 + usbipd-win） |
| WSL **picocom** 读日志 | `picocom -b <固件波特> /dev/ttyUSB*`（见上节） |
| Windows **Agent** 读日志 | `./scripts/serial-ch341-read.sh [--baud N]`（自动 COM） |

---

## 运行环境要求

- **Windows**：必须在 **Git Bash** 中执行（`scripts/lib/os-detect.sh` 强制）
- **Linux / macOS**：bash 4+

---

### fetch-stm32f103-docs.sh — ST 官方文档下载

下载 **DS5319** Datasheet 与 **RM0008** Reference Manual 至 `doc/reference/stm32f103/pdf/`。  
**ST 官网无需注册**；若 curl 超时，可用浏览器打开直链保存 PDF 后 `--verify-only` 校验。

```bash
./scripts/fetch-stm32f103-docs.sh
./scripts/fetch-stm32f103-docs.sh --verify-only
./scripts/fetch-stm32f103-docs.sh --force
./scripts/fetch-stm32f103-docs.sh --no-proxy
```

- 优先 ST 直链；RM0008 失败时尝试 Keil 镜像  
- SHA256 写入 `doc/reference/stm32f103/checksums.sha256`  
- 精选 MD 见 [reference/stm32f103/md/](reference/stm32f103/md/)

---

### fetch-cmsis.sh — CMSIS + HAL 子模块（Core + Device F1 + HAL）

初始化或更新 ST CMSIS + HAL **git submodules**：

| 路径 | Tag | 层 |
|------|-----|-----|
| `vendor-pack/cmsis-core/` | 分支 `cm3` · `v5.6.0_cm3` | CMSIS-Core（Cortex-M3，F103C8T6） |
| `vendor-pack/cmsis-device-f1/` | `v4.3.5` | CMSIS-Device F1（F103xB） |
| `vendor-pack/stm32f1xx-hal-driver/` | `v1.1.8` | HAL/LL F1 |

```bash
git clone --recursive <repo-url>
./scripts/fetch-cmsis.sh
./scripts/fetch-cmsis.sh --verify-only
```

- 验证：Core `Include/core_cm3.h`；Device `Include/stm32f103xb.h`、`Source/Templates/gcc/startup_stm32f103xb.s`、`Source/Templates/system_stm32f1xx.c`；HAL `Inc/stm32f1xx_hal.h`、`Src/stm32f1xx_hal_gpio.c`
- 说明：[cmsis-core.embed-dev-lab.md](../vendor-pack/cmsis-core.embed-dev-lab.md)、[cmsis-device-f1.embed-dev-lab.md](../vendor-pack/cmsis-device-f1.embed-dev-lab.md)、[stm32f1xx-hal-driver.embed-dev-lab.md](../vendor-pack/stm32f1xx-hal-driver.embed-dev-lab.md)
- 组件归纳：[doc/learn/stm32-cmsis-component-repos.md](learn/stm32-cmsis-component-repos.md)
- 兼容：`fetch-cmsis-core.sh` 为同脚本的别名入口

---

### fetch-f103-cmsis-hal-deps.sh — f103-cmsis-hal 最小 CMSIS/HAL

从 `vendor-pack` CMSIS + HAL submodule（`stm32f1xx-hal-driver@v1.1.8`）拷贝 PC13 闪烁所需最小子集至 `projects/f103-cmsis-hal/third_party/`，并拷贝 CMSIS `startup` / `system` / `STM32F103XB_FLASH.ld`（C8 64K 裁剪）。

```bash
./scripts/fetch-cmsis.sh
./scripts/fetch-f103-cmsis-hal-deps.sh
./scripts/fetch-f103-cmsis-hal-deps.sh --verify-only
```

构建：`./scripts/build.sh f103-cmsis-hal`。说明见 [doc/projects/f103-cmsis-hal.md](projects/f103-cmsis-hal.md)。

---

### fetch-stm32cubef1.sh — STM32CubeF1 固件包

获取 ST [**STM32CubeF1**](https://github.com/STMicroelectronics/STM32CubeF1) MCU Package（CMSIS/HAL/Middleware/例程）至 `vendor-pack/STM32CubeF1/`。  
HAL 独立上游：[stm32f1xx-hal-driver](https://github.com/STMicroelectronics/stm32f1xx-hal-driver)（见 [`stm32f1xx-hal-driver.embed-dev-lab.md`](../vendor-pack/stm32f1xx-hal-driver.embed-dev-lab.md)）。  
GitHub「Download ZIP」不含 submodule；请用 **ST 官网 ZIP** 或脚本的 **`--clone`**。

```bash
./scripts/fetch-stm32cubef1.sh
./scripts/fetch-stm32cubef1.sh --from-zip vendor-pack/STM32CubeF1/archives/STM32CubeF1-1.8.6.zip
./scripts/fetch-stm32cubef1.sh --clone
./scripts/fetch-stm32cubef1.sh --verify-only
./scripts/fetch-stm32cubef1.sh --force
```

- 默认：若 `archives/*.zip` 存在则解压，否则 `git clone --recursive` v1.8.6  
- 验证：`Drivers/CMSIS/.../startup_stm32f103xb.s` 与 `system_stm32f1xx.c`（旧版为 `system_stm32f1xx.c`）  
- 详见 [vendor-pack/STM32CubeF1/README.md](../vendor-pack/STM32CubeF1/README.md)

---

### install-mcp-skills.sh — MCP 与项目 Skill

安装 **embedded-debugger-mcp**（cargo 构建 + probe-rs）与 **embed-dev-lab** Skill。详见 [mcp-skills.md](mcp-skills.md)。

```bash
./scripts/install-mcp-skills.sh
./scripts/install-mcp-skills.sh --global
./scripts/install-mcp-skills.sh --verify-only
./scripts/bootstrap.sh --with-mcp    # bootstrap 可选步骤
```

---

## 相关文档

- [快速上手](getting-started.md)
- [probe-rs](probe-rs.md)
- [IDE 调试](ide-debug.md)
