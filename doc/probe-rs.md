# probe-rs 使用指南

本文说明 **probe-rs CLI** 在本项目中的安装、ST-Link 驱动、烧录与排错。终端命令与 CLI 输出为英文；文档正文为中文。

## 角色

| 组件 | 用途 |
|------|------|
| `probe-rs` CLI | 列出调试器、烧录 ELF、复位目标 |
| `probe-rs.probe-rs-debugger` 插件 | IDE 图形化调试（见 [ide-debug.md](ide-debug.md)） |

本项目 F103 demo 的 chip 名：**`STM32F103C8Tx`**（定义于 `scripts/build.sh`）。

---

## 安装

### 自动（推荐）

在 Git Bash 仓库根目录：

```bash
./scripts/bootstrap.sh
# 或仅安装工具
./scripts/install-tools.sh
```

各 OS 安装逻辑见 `scripts/install/windows.sh`、`linux.sh`、`macos.sh`。

### 验证

```bash
probe-rs list
```

期望输出示例：

```text
The following debug probes were found:
[0]: STLink V2 -- 0483:3748: (ST-LINK)
```

也可运行：

```bash
./scripts/env-check.sh
```

Windows 下会额外检查 Debug Probe（optional）。

---

## ST-Link 驱动（按 OS）

### Windows — WinUSB

probe-rs 需要 ST-Link **Debug 接口** 绑定 **WinUSB**（不是仅 ST 官方驱动名「STM32 STLink」）。

**主路径 — 仓库 bundled 驱动包：**

```bash
./scripts/install/stlink-winusb-windows.sh --check-only
./scripts/install/stlink-winusb-windows.sh --install   # UAC 管理员
```

驱动文件目录：`vendor-pack/STLink/STLink/USBDriver/`

手动步骤：

1. **拔掉** ST-Link USB
2. 管理员运行 `stlink_winusb_install.bat`（readme：**先装驱动再插设备**）
3. 插入 ST-Link，执行 `probe-rs list`

**备选 — Zadig**（ST-Link V3 或 INF 未覆盖的 PID）：

1. 打开 [https://zadig.akeo.ie/](https://zadig.akeo.ie/)
2. Options → List All Devices
3. 选择 **ST-Link Debug** → Driver **WinUSB** → Replace Driver

### Linux — udev

```bash
sudo cp scripts/install/assets/99-probe-rs.rules /etc/udev/rules.d/99-probe-rs.rules
sudo udevadm control --reload-rules
sudo udevadm trigger
```

`linux.sh` 安装结束时会打印上述提示。

### macOS

无需额外 ST-Link 驱动。

---

## 硬件连接

### USB（调试器 ↔ PC）

- ST-Link 的 **USB 口** 必须接 PC（可经 USB Hub / 扩展坞；本项目实机验证绿联 USB 扩展可用）
- **USB 枚举** 与 SWD 四线无关；`probe-rs list` 只看 USB

### SWD（调试器 ↔ F103 核心板）

| ST-Link | F103 | 说明 |
|---------|------|------|
| SWDIO | PA13 | 数据 |
| SWCLK | PA14 | 时钟 |
| GND | GND | 必须共地 |
| 3.3V | 3.3V | 板子另有稳定 3.3V 供电时可只接前三线 |

烧录前确认板子 **3.3V 供电**，否则可能写 Flash 成功但程序不运行。

---

## 常用 CLI

### 列出调试器

```bash
probe-rs list
```

### 烧录 ELF

与 `scripts/build.sh f103-manual-reg flash` 等价：

```bash
probe-rs download --chip STM32F103C8Tx --binary-format elf projects/f103-manual-reg/build/f103-manual-reg.elf
probe-rs reset --chip STM32F103C8Tx
```

> **注意**：probe-rs 新版本使用 `--binary-format elf`，**不是** 旧参数 `--format elf`。

### 探测芯片（可选）

```bash
probe-rs chip info --chip STM32F103C8Tx
```

### 一键脚本烧录

```bash
./scripts/build.sh f103-manual-reg build
./scripts/build.sh f103-manual-reg flash
```

`flash` **不会**自动编译；修改源码后须先 `build`。

---

## ELF 与 HEX 的关系

| 项目 | 说明 |
|------|------|
| 工具链 | **`arm-none-eabi-gcc`** 裸机链，非 `arm-linux-gnueabihf` |
| `.elf` | Ninja 链接产物；**probe-rs 与本项目主烧录路径使用 ELF** |
| `.hex` | `build` 时由 [`cmake/mcu-config.cmake`](../cmake/mcu-config.cmake) POST_BUILD 调用 `arm-none-eabi-objcopy -O ihex` 自动生成 |
| `.bin` | 当前工程**不生成** |
| OpenOCD | `./scripts/build.sh f103-manual-reg flash-openocd` 烧录 **`.hex`**，须先 `build` |

---

## 故障排查

| 现象 | 可能原因 | 处理 |
|------|----------|------|
| `No debug probes were found` | WinUSB 未绑定 / USB 未枚举 | WinUSB 安装；换 USB 线/口；`stlink-winusb-windows.sh --check-only` |
| 设备管理器无 `VID_0483` | USB 线/Hub/供电 | 直连主板 USB 2.0；对照拔插 ST-Link |
| `unexpected argument '--format'` | CLI 版本过新 | 改用 `--binary-format elf` |
| 烧录成功 LED 不闪 | 未 build 最新 elf / 未 reset / SWD 或供电 | `build && flash`；按 RESET；查 SWD 与 3.3V |
| 程序完全不运行 | PC13 未开 DBP | 见 [f103-manual-reg 模块](projects/f103-manual-reg.md) Backup 域说明 |
| IDE 找不到 probe-rs | PATH 未进 Cursor | `./scripts/setup-path.sh` 后重启 Cursor |

---

## 相关文档

- [快速上手](getting-started.md)
- [IDE 调试与插件](ide-debug.md)
- [脚本说明](scripts-reference.md)
