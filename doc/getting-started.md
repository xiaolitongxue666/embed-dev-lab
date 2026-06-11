# 快速上手

从零开始在 **STM32F103C8T6 核心板** 上点亮 PC13 LED。命令与 CLI 输出为英文。

## 前置

| 项目 | 说明 |
|------|------|
| OS | Windows（Git Bash）/ Linux / macOS |
| IDE | Cursor 或 VS Code（推荐） |
| 硬件 | ST-Link V2、F103 核心板、USB 线、杜邦线 |

Windows 请安装 [Git for Windows](https://git-scm.com/download/win)，Cursor 终端默认 **Git Bash**。

---

## 步骤 1：克隆并进入仓库

```bash
cd /path/to/embed-dev-lab
```

## 步骤 2：一键环境 + 编译

```bash
./scripts/bootstrap.sh
```

等价于：安装工具 → 配置 PATH → 安装扩展 → 校验 → 编译 `f103-blink` → 配置 clangd。

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

**USB**：ST-Link USB → PC

**SWD**（四线）：

| ST-Link | 板子 |
|---------|------|
| SWDIO | PA13 |
| SWCLK | PA14 |
| GND | GND |
| 3.3V | 3.3V |

确保板子 **3.3V 供电**。

## 步骤 5：编译 + 烧录

```bash
./scripts/build.sh f103-blink build
./scripts/build.sh f103-blink flash
```

或 Run Task → **Build and Flash F103**。

## 步骤 6：验证

PC13 连接 LED 应约 **1 秒周期闪烁**（多数板子低电平点亮）。

若无闪烁：按板载 **RESET**；见 [modules-f103-blink.md](modules-f103-blink.md) 与 [probe-rs.md 排错](probe-rs.md#故障排查)。

---

## 可选：IDE 调试

1. F5 → **F103 Probe-rs Debug**
2. 自动编译、烧录、断点

详见 [ide-debug.md](ide-debug.md)。

---

## 下一步

- [probe-rs 详细说明](probe-rs.md)
- [脚本完整参考](scripts-reference.md)
- [f103-blink 源码说明](modules-f103-blink.md)

维护速查：[PROJECT_MEMORY.md](../PROJECT_MEMORY.md)
