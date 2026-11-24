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

### SWD（调试器 ↔ F103 蓝板）

蓝板由 **MicroUSB 独立供电**；ST-Link **不要**把 3.3V/5V 接到蓝板电源脚。详解：[供电与共地](hardware/power-and-common-ground.md)。

| ST-Link | F103 | 说明 |
|---------|------|------|
| SWDIO | PA13 | 数据 |
| SWCLK | PA14 | 时钟 |
| GND | 面包板 GND 轨 → 蓝板 SWD GND | **必须**星型共地（CH341 也直接进该轨） |

烧录前确认蓝板 **MicroUSB 已供电**，否则可能写 Flash 成功但程序不运行。

接外设时：ST-Link 3.3V → 面包板电源轨（方案 A：5V 闲置）。

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

烧写脚本位置（主路径）：

| 文件 | 作用 |
|------|------|
| [`scripts/build.sh`](../scripts/build.sh) | `flash` → probe-rs download + reset；`flash-openocd` 烧 HEX |
| [`scripts/build-flash.sh`](../scripts/build-flash.sh) | 一键 build + flash（不 configure） |

```bash
./scripts/build.sh f103-manual-reg build
./scripts/build.sh f103-manual-reg flash
# 或
./scripts/build-flash.sh f103-manual-reg
```

`flash` **不会**自动编译；修改源码后须先 `build`。完整地图见 [workflow-write-build-flash.md § 烧写脚本在哪里](workflow-write-build-flash.md#烧写脚本在哪里)。

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
| `No debug probes were found` | WinUSB 未绑定 / USB 未枚举 | 先看是不是描述符失败（下一行）。已经是 `0483:3748` 仍打不开时，再 `stlink-winusb-windows.sh --check-only` |
| 同一 USB 口变成 `VID_0000&PID_0002`，状态为错误，文案是设备描述符请求失败 | 枚举没完成，还不是 ST-Link | `pnputil /restart-device "<该实例 ID>"`。不要对 `0000:0002` 装 WinUSB。起来之后 `probe-rs list` 应为 `STLink V2 -- 0483:3748`。2026-11-24 这一口重启后恢复 |
| 设备管理器无 `VID_0483` | USB 线/Hub/供电，或仍停在上一行的失败节点 | 直连主板 USB 2.0；对照拔插 ST-Link |
| `Failed to open probe` / USB error 5 | 另一个 probe-rs 或 gdb 还占着 ST-Link | 结束 `probe-rs.exe` 与 `arm-none-eabi-gdb.exe` 后再下载。`gdb -batch` 里的 `while` 会掉进交互提示并一直占着探针 |
| `unexpected argument '--format'` | CLI 版本过新 | 改用 `--binary-format elf` |
| 烧录成功 LED 不闪 | 未 build 最新 elf / 未 reset / SWD 或供电 | `build && flash`；按 RESET；查 SWD 与 3.3V |
| 程序完全不运行 | PC13 未开 DBP | 见 [f103-manual-reg 模块](projects/f103-manual-reg.md) Backup 域说明 |
| IDE 找不到 probe-rs | PATH 未进 Cursor | `./scripts/setup-path.sh` 后重启 Cursor |

## 停核读寄存器

`probe-rs read` 只接受 RAM 地址。USART、RCC、SCB 用 gdb，并且先关掉「默认不可访问」：

```bash
probe-rs gdb --chip STM32F103C8Tx --non-interactive projects/<module>/build/<module>.elf \
  --gdb arm-none-eabi-gdb -- -batch \
  -ex "set pagination off" \
  -ex "set mem inaccessible-by-default off" \
  -ex "info registers pc lr xpsr psp msp" \
  -ex "x/1wx 0xE000ED28" \
  -ex "x/1wx 0xE000ED2C" \
  -ex "detach"
probe-rs reset --chip STM32F103C8Tx
```

`0xE000ED28` 是 CFSR，`0xE000ED2C` 是 HFSR。没加 `set mem inaccessible-by-default off` 时，这两处和 `0x40013800`（USART1）会报 `Cannot access memory`。Handler 的 LR 为 `0xFFFFFFFD` 时，异常帧在 PSP：从 PSP 起 8 个字是 R0、R1、R2、R3、R12、LR、PC、xPSR。`probe-rs read -o` 写出的是 Intel HEX，不是原始二进制。看完执行 `probe-rs reset`，detach 可能把核留在暂停。

---

## 相关文档

- [快速上手](getting-started.md)
- [IDE 调试与插件](ide-debug.md)
- [脚本说明](scripts-reference.md)
