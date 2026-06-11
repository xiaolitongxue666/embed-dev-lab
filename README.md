# embed-dev-lab

*Cross-platform embedded lab: CMake + Ninja + ARM GCC + probe-rs*

跨平台嵌入式开发实验框架：**CMake + Ninja + ARM GCC + probe-rs**（主）/ OpenOCD（备选）。  
Demo 目标：**STM32F103C8T6** PC13 LED 闪烁（`modules/f103-blink`）。

**详细文档** → [`doc/README.md`](doc/README.md)

---

## 快速开始

```bash
# 1. 一键：装工具 + 编译 demo（不含烧录）
./scripts/bootstrap.sh

# 2. 确认 ST-Link（Windows 见 doc/probe-rs.md）
probe-rs list

# 3. 编译 + 烧录
./scripts/build.sh f103-blink build && ./scripts/build.sh f103-blink flash
```

完整步骤：[doc/getting-started.md](doc/getting-started.md)

---

## 项目结构

```text
embed-dev-lab/
├── doc/                        # 使用文档（中文）
│   ├── README.md               # 文档索引
│   ├── getting-started.md      # 快速上手
│   ├── probe-rs.md             # probe-rs CLI / ST-Link / WinUSB
│   ├── ide-debug.md            # IDE 扩展与调试
│   ├── scripts-reference.md    # 脚本说明与自动化链路
│   └── modules-f103-blink.md   # F103 demo 说明
├── scripts/                    # 安装、PATH、构建、校验
│   ├── bootstrap.sh            # 一键环境 + 编译
│   ├── build.sh                # 模块 configure / build / flash
│   ├── install-tools.sh        # 分 OS 安装入口
│   ├── env-check.sh            # 工具链与扩展校验
│   ├── setup-path.sh           # User PATH + bashrc
│   ├── setup-clangd.sh         # compile_commands + clangd 配置
│   ├── install-extensions.sh   # 安装 .vscode/extensions.json
│   ├── install/                # windows | linux | macos | stlink-winusb
│   └── lib/                    # 公共 shell 库
├── cmake/
│   ├── toolchain-arm-none-eabi.cmake
│   └── mcu-config.cmake        # embed_mcu_add_executable()
├── modules/
│   └── f103-blink/             # STM32F103C8T6 PC13 闪烁
│       ├── src/
│       ├── startup/
│       └── linker/
├── install_packet/             # ST-Link WinUSB 驱动、核心板资料（参考用）
├── .vscode/                    # launch.json / tasks.json / extensions.json
├── PROJECT_MEMORY.md           # 维护速查（Agent）
└── README.md
```

---

## 脚本与自动化

| 脚本 | 作用 | 是否烧录 |
|------|------|----------|
| `bootstrap.sh` | 装工具 → PATH → 扩展 → 校验 → **编译** f103-blink → clangd | 否 |
| `build.sh f103-blink` | configure + build | 否 |
| `build.sh f103-blink flash` | probe-rs 烧录 + reset | 是（**不自动 compile**） |
| `build.sh f103-blink build && flash` | 编译后烧录 | 是 |

链路图与全部选项：[doc/scripts-reference.md](doc/scripts-reference.md)

### bootstrap 常用参数

```bash
./scripts/bootstrap.sh --build-only          # 跳过安装
./scripts/bootstrap.sh --install-only      # 只安装，不编译
./scripts/bootstrap.sh --skip-extensions     # 不装 IDE 扩展
./scripts/bootstrap.sh --module f103-blink   # 指定模块（默认 f103-blink）
```

---

## 编译与烧录

### 方式对比

| 方式 | 自动编译 | 自动烧录 | 断点调试 |
|------|:--------:|:--------:|:--------:|
| CLI `build && flash` | 手动两步 | 是 | 否 |
| Task **Build and Flash F103** | 是 | 是 | 否 |
| **F103 Probe-rs Debug**（F5） | 是 | 是 | 是 |
| `bootstrap.sh` | 是 | 否 | 否 |

### CLI

```bash
./scripts/build.sh f103-blink              # configure + build
./scripts/build.sh f103-blink build
./scripts/build.sh f103-blink flash
./scripts/build.sh f103-blink flash-openocd   # 需 openocd
./scripts/build.sh f103-blink clean
```

### IDE

1. 安装扩展：`./scripts/install-extensions.sh`（或 bootstrap 默认安装）
2. F5 → **F103 Probe-rs Debug**（自动 build + flash）

详见 [doc/ide-debug.md](doc/ide-debug.md)。

---

## ST-Link / probe-rs（摘要）

Windows 上 probe-rs 需要 ST-Link **Debug** 接口 **WinUSB**：

```bash
./scripts/install/stlink-winusb-windows.sh --check-only
./scripts/install/stlink-winusb-windows.sh --install
probe-rs list
```

驱动包：`install_packet/STLink/STLink/USBDriver/`  
完整说明：[doc/probe-rs.md](doc/probe-rs.md)

---

## 依赖工具

| 工具 | 用途 |
|------|------|
| Git Bash | **Windows 必须** — 所有脚本在此运行 |
| CMake ≥ 3.20 + Ninja | 构建 |
| arm-none-eabi-gcc | 交叉编译 |
| clangd | LSP |
| probe-rs | 烧录 / 调试（主） |
| OpenOCD | 备选 |
| Cursor / VS Code | IDE |

---

## 文档索引

| 文档 | 说明 |
|------|------|
| [doc/getting-started.md](doc/getting-started.md) | 从零到 LED 闪烁 |
| [doc/probe-rs.md](doc/probe-rs.md) | probe-rs CLI、驱动、排错 |
| [doc/ide-debug.md](doc/ide-debug.md) | probe-rs-debugger 等扩展 |
| [doc/scripts-reference.md](doc/scripts-reference.md) | 脚本完整参考 |
| [doc/modules-f103-blink.md](doc/modules-f103-blink.md) | F103 demo 模块 |

---

## PATH 说明（Windows）

工具写入 **User PATH** 与 Git Bash `~/.bashrc`（`# >>> embed-dev-lab PATH >>>`）。  
Cursor 扩展若找不到工具：运行 `./scripts/setup-path.sh` 并 **重启 Cursor**。

---

## 新增模块

1. 在 `modules/<name>/` 添加 `CMakeLists.txt`、`CMakePresets.json`、源码  
2. 使用 `embed_mcu_add_executable()`（`cmake/mcu-config.cmake`）  
3. `./scripts/build.sh <name>`

---

## License

MIT — see [LICENSE](LICENSE).
