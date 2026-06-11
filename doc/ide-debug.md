# IDE 调试与扩展

本文说明 Cursor / VS Code 中与本项目相关的扩展、启动配置与 **自动编译 + 烧录** 调试流程。

## 推荐扩展

来源：`.vscode/extensions.json`

| 扩展 ID | 必需 | 作用 |
|---------|------|------|
| `probe-rs.probe-rs-debugger` | 是 | **主调试器**：probe-rs 图形化调试、烧录 |
| `llvm-vs-code-extensions.vscode-clangd` | 是 | C/C++ 语言服务（LSP） |
| `ms-vscode.cmake-tools` | 是 | CMake Presets 集成 |
| `marus25.cortex-debug` | 否 | OpenOCD + Cortex-Debug 备选 |

已禁用 IntelliSense 冲突：`ms-vscode.cpptools` 在 `unwantedRecommendations` 中。

### 安装扩展

```bash
./scripts/bootstrap.sh                  # 默认安装扩展
./scripts/install-extensions.sh         # 仅扩展
./scripts/bootstrap.sh --skip-extensions  # 跳过
```

---

## probe-rs-debugger 使用

### 前置条件

1. `probe-rs list` 能列出 ST-Link（见 [probe-rs.md](probe-rs.md)）
2. 已安装扩展 `probe-rs.probe-rs-debugger`
3. 硬件 SWD + 供电已接好

### 一键调试（自动编译 + 烧录）

1. 打开 Run and Debug（运行和调试）
2. 选择配置：**F103 Probe-rs Debug**
3. 按 F5 或点击 Start Debugging

流程：

```text
preLaunchTask "Build F103"
  → ./scripts/build.sh f103-blink    # 自动 configure + build
  → probe-rs-debugger launch
  → flashingConfig: 烧录 f103-blink.elf
  → haltAfterReset + connectUnderReset: 复位后停于 main
```

### launch.json 要点

文件：`.vscode/launch.json`

| 字段 | 值 | 含义 |
|------|-----|------|
| `type` | `probe-rs-debug` | 使用 probe-rs 插件 |
| `chip` | `STM32F103C8Tx` | 目标芯片 |
| `preLaunchTask` | `Build F103` | 调试前自动编译 |
| `flashingConfig.flashingEnabled` | `true` | 启动时烧录 |
| `flashingConfig.formatOptions.binaryFormat` | `elf` | ELF 格式 |
| `coreConfigs[0].programBinary` | `modules/f103-blink/build/f103-blink.elf` | 程序文件 |
| `connectUnderReset` | `true` | 在复位下连接 |

### 与 CLI 的差异

| 方式 | 自动编译 | 自动烧录 | 断点调试 |
|------|----------|----------|----------|
| **F103 Probe-rs Debug** | 是（preLaunchTask） | 是（flashingConfig） | 是 |
| `build.sh flash` | **否** | 是 | 否 |
| Task **Build and Flash F103** | 是 | 是 | 否 |

---

## Tasks（终端任务）

文件：`.vscode/tasks.json`

| Task 标签 | 命令 | 用途 |
|-----------|------|------|
| **Build F103** | `./scripts/build.sh f103-blink` | 默认构建任务（Ctrl+Shift+B） |
| **Build and Flash F103** | `build` + `flash` | 编译并烧录，不进入调试 |
| **Flash OpenOCD** | `./scripts/build.sh f103-blink flash-openocd` | OpenOCD 烧录 |
| **Env Check** | `./scripts/env-check.sh` | 环境校验 |

运行 Task：Terminal → Run Task…

---

## OpenOCD 备选调试

需安装 `openocd` 且扩展 `marus25.cortex-debug` 已安装。

1. 选择 **F103 OpenOCD Debug**
2. 使用 `interface/stlink.cfg` + `target/stm32f1x.cfg`
3. 同样依赖 **Build F103** preLaunchTask

---

## clangd / CMake

| 设置 | 文件 | 说明 |
|------|------|------|
| `terminal.integrated.defaultProfile.windows` | `.vscode/settings.json` | Git Bash |
| `cmake.usePresets` | `.vscode/settings.json` | 使用 CMake Presets |
| `compile_commands.json` | 仓库根目录 | `build.sh` / `bootstrap.sh` 后由 `setup-clangd.sh` 同步 |
| `settings.local.json` | `.vscode/` | clangd 路径（自动生成，已 gitignore） |

---

## 常见问题

| 问题 | 处理 |
|------|------|
| 扩展未安装 / bootstrap 报 extension failed | 运行 `./scripts/install-extensions.sh`；必需扩展失败时 bootstrap 会退出；无 GUI 环境用 `--skip-extensions` |
| F5 提示找不到 probe-rs | User PATH 添加 cargo/WinGet 路径；重启 Cursor |
| 调试前 build 失败 | 终端单独运行 `./scripts/build.sh f103-blink` 看错误 |
| 烧录后无断点停住 | 确认 `haltAfterReset: true`；检查 SWD |
| Git Bash 路径错误 | 工作区已设 profile 名；User settings 中配置实际 bash 路径 |

---

## 相关文档

- [probe-rs CLI 与驱动](probe-rs.md)
- [脚本参考](scripts-reference.md)
- [f103-blink 模块](modules-f103-blink.md)
