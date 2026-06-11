# embed-dev-lab — Project Memory

> 项目级持久知识（仅本仓库）。最后更新：2026-06-11

## 快速路径

| 用途 | 路径 / 命令 |
|------|-------------|
| 一键环境 + 编译 | `./scripts/bootstrap.sh` |
| F103 编译 | `./scripts/build.sh f103-blink build` |
| F103 烧录 | `./scripts/build.sh f103-blink flash` |
| ST 官方文档 fetch | `./scripts/fetch-stm32f103-docs.sh` |
| STM32CubeF1 fetch | `./scripts/fetch-stm32cubef1.sh` |
| ST 官方参考（PDF+MD） | `doc/reference/stm32f103/` |
| 裸机入门笔记 | `doc/learn/stm32-bare-metal-bootstrap.md` |
| 厂商本地资料 | `vendor-pack/`（驱动 / 板级例程 / CubeF1） |
| MCP / Skill 安装 | `./scripts/install-mcp-skills.sh` |
| MCP 校验 | `./scripts/install-mcp-skills.sh --verify-only` |
| ST-Link WinUSB (Windows) | `./scripts/install/stlink-winusb-windows.sh --check-only` |
| 驱动包 | `vendor-pack/STLink/STLink/USBDriver/` |
| 板级 PC13 参考例程 | `vendor-pack/.../核心板测试程序(PC13闪烁)/` |
| probe-rs chip | `STM32F103C8Tx` |

## 编号事实（≤25）

1. **Windows 脚本入口**：Git Bash；`scripts/lib/os-detect.sh` 在非 Windows 拒绝运行需 Git Bash 的逻辑。
2. **PATH**：工具写入 User PATH + `~/.bashrc` 的 `# >>> embed-dev-lab PATH >>>`；IDE 扩展需重启终端或 Cursor 才可见。
3. **代理**：`scripts/lib/proxy.sh` 默认 `http://127.0.0.1:7890`；bootstrap/install-extensions/fetch 脚本/install-mcp-skills 会应用。
4. **主烧录链**：probe-rs（首选）；OpenOCD 可选，`env-check` 中 openocd 为 optional。
5. **probe-rs 烧录 CLI**：`probe-rs download --chip STM32F103C8Tx --binary-format elf <elf>`；旧版 `--format` 已废弃。
6. **烧录后复位**：`scripts/build.sh` 的 `flash` 在 download 后执行 `probe-rs reset`，避免目标停在调试态。
7. **ST-Link WinUSB**：Windows 上 probe-rs 需 Debug 接口 WinUSB；bundled 路径 `vendor-pack/STLink/.../USBDriver/`，脚本 `stlink-winusb-windows.sh`。
8. **f103-blink 时钟**：`system_stm32f10x.c` 手写寄存器版（非 CubeMX）；HSE→72MHz，HSE 超时保持 HSI。
9. **PC13 / Backup 域**：`RCC_APB1ENR.PWREN` + `PWR_CR.DBP` 后再写 `GPIOC_CRH`；详见 `doc/reference/stm32f103/md/topics/backup-domain-pc13.md`。
10. **启动文件**：`startup_stm32f103xb.s` 为 CMSIS 风格启动流程（`.data`/`.bss` → `SystemInit` → `main`）；demo 内为精简手写版。
11. **flash 前需 build**：`build.sh f103-blink flash` 不自动编译。
12. **ST 官方 PDF+MD**：`doc/reference/stm32f103/`；PDF 在 `pdf/`（gitignore）；`fetch-stm32f103-docs.sh`（ST 直链，RM Keil 镜像）；topic 页码以本地 PDF Rev 为准。
13. **vendor-pack**（原 install_packet）：ST-Link 驱动（可提交）+ 核心板 MDK 例程 + STM32CubeF1；与 `doc/reference/` 分工不同。
14. **fetch-stm32cubef1.sh**：优先 `vendor-pack/STM32CubeF1/archives/*.zip` 解压，否则 `git clone --recursive v1.8.6`；**勿用 GitHub「Download ZIP」**（缺 submodule）。
15. **CMSIS 模板路径**（CubeF1 fetch 后）：`.../Templates/gcc/startup_stm32f103xb.s`；system 新版为 **`system_stm32f1xx.c`**（旧包可能仍为 `system_stm32f10x.c`）；f103-blink **不链接** Cube 包构建。
16. **用户文档**：`doc/` 中文；入口 `doc/README.md`；脚本 `doc/scripts-reference.md`；学习笔记 `doc/learn/stm32-bare-metal-bootstrap.md`（RCC/HSE/startup/CMSIS 问答）。
17. **USB / ST-Link**：绿联 Hub 下 ST-Link V2 (`0483:3748`) 可正常枚举；SWD 四线 SWDIO/SWCLK/GND/3.3V。
18. **clangd**：build/bootstrap 后 `setup-clangd.sh` 同步根 `compile_commands.json`。
19. **调试**：Cursor Run →「F103 Probe-rs Debug」；`.vscode/launch.json` 已配置。
20. **f103-blink 注释**：源码中文；终端/CLI 输出保持英文。
21. **`.gitignore`**：`vendor-pack/STM32CubeF1/`（除 README）、核心板目录、PDF、`.tools/` 忽略；`STLink/USBDriver` 可提交。
22. **MCP/Skill**：`install-mcp-skills.sh` 构建 embedded-debugger-mcp → `.cursor/mcp.json` + `skills/embed-dev-lab`；`--global` 合并多 Agent；Codex 无 MCP。
23. **embedded-debugger 实机验证**（2026-06-11）：`list_probes` / connect / reset / read_memory 均通过；Cursor Agent 需 MCP Connected + 重载/新对话。
24. **Cursor 终端**：工作区 `.vscode/settings.json` 声明 `defaultProfile: Git Bash`。
25. **实机验证**：ST-Link V2 + SWD + 3.3V，PC13 LED 正常闪烁（Backup 域 DBP 修复后）；`probe-rs download` + `reset` 约 0.3s。

## 问题 ↔ 解法

| 问题 | 解法 |
|------|------|
| `probe-rs list` 为空 | Windows：`stlink-winusb-windows.sh --install` 或 Zadig WinUSB |
| 烧录成功但 PC13 不闪 | PWR+DBP；先 `build` 再 `flash`；`probe-rs reset` |
| 程序卡死、无任何 IO | HSE 超时逻辑已在 `system_stm32f10x.c` |
| `unexpected argument '--format'` | 改用 `--binary-format elf` |
| ST PDF curl 超时/SSL 失败 | 浏览器保存至 `pdf/`；RM0008 可用 Keil 镜像；`--verify-only` |
| RM0008 topic 页码对不上 | 核对本地 PDF 页脚 Rev（Rev 9 vs Rev 21） |
| GitHub STM32CubeF1「Download ZIP」缺文件 | `git clone --recursive` 或 ST 官网 ZIP + `fetch-stm32cubef1.sh` |
| fetch CubeF1 报缺 `system_stm32f10x.c` | 新版 CMSIS 为 `system_stm32f1xx.c`；脚本 `--verify-only` 已适配 |
| 文档/脚本仍写 `install_packet` | 已更名为 **`vendor-pack/`** |
| bootstrap 慢 | `--build-only --skip-extensions` |
| Cursor 找不到 cmake/clangd | `setup-path.sh`，重启 Cursor |
| IDE clangd 报错 | 先 `build`，再 `setup-clangd.sh` |
| MCP `cargo required` | 安装 Rust（rustup.rs），再 `./scripts/install-mcp-skills.sh` |
| Cursor 无 embedded-debugger | 运行 install 脚本；MCP Connected；Reload Window 或新对话 |
| Cursor Agent 调不到 MCP 工具 | 确认 MCP 已连接；非 Codex（Codex 无 MCP）；新开 Agent 对话 |
| Windows Python 测 MCP 编码/启动失败 | `uv run python`；subprocess 直调 `.exe`；stdin/stdout UTF-8 字节流 |
