# embed-dev-lab — Project Memory

> 项目级持久知识（仅本仓库）。最后更新：2026-08-26（summary-memory：CH341 Win 读串口 / 规则）

## 快速路径

| 用途 | 路径 / 命令 |
|------|-------------|
| 编写 → 编译 → 下载 | `doc/workflow-write-build-flash.md`（含「烧写脚本在哪里」） |
| Agent 规则 / Skill | `AGENTS.md` · `skills/embed-dev-lab/SKILL.md` · `.cursor/rules/`（含 `serial-ch341.mdc`） |
| 一键环境 | `./scripts/bootstrap.sh`（无代理 `--no-proxy`；**不含烧录**） |
| 烧写入口 | `scripts/build.sh`（真正 flash）· `scripts/build-flash.sh`（一键）· IDE：`.vscode/tasks.json` / `launch.json` |
| CH341 串口 | `serial-ch341-switch.sh`；Win Agent：`serial-ch341-read.sh [--baud]`（自动 COM）；WSL：`picocom -b <固件波特>` |
| 编译 / 烧录命令 | `./scripts/build.sh <module> build\|flash`；一键 `build-flash.sh`（不 configure） |
| cmsis-hal 依赖 | `./scripts/fetch-cmsis.sh` → `fetch-f103-cmsis-hal-deps.sh` |
| 工程文档 | `doc/projects/` · `projects/*/README.md` |
| 上手 / 脚本 / probe | `doc/getting-started.md` · `doc/scripts-reference.md` · `doc/probe-rs.md` |
| 学习笔记入口 | `doc/README.md`（含 VMA/LMA、newlib、CMSIS、内存映射） |
| ST 官方参考 | `doc/reference/stm32f103/`；fetch：`fetch-stm32f103-docs.sh` |
| CMSIS+HAL submodule | `./scripts/fetch-cmsis.sh`（core `v5.6.0_cm3` / device `v4.3.5` / HAL `v1.1.8`） |
| WinUSB / MCP | `stlink-winusb-windows.sh` · `install-mcp-skills.sh` |
| IDE F5 | `doc/ide-debug.md` |

## 编号事实（≤25）

1. **Windows**：脚本须 **Git Bash**（`os-detect.sh`）。
2. **PATH**：User PATH + `~/.bashrc` embed-dev-lab 块；IDE 需重启。
3. **代理**：默认 `http://127.0.0.1:7890`；`--no-proxy` 可关。
4. **烧录**：probe-rs；chip `STM32F103C8Tx`；`--binary-format elf`；flash 后 `probe-rs reset`。
5. **flash 不 compile / 不 configure**：改代码先 `build`；`clean` 后先 `./scripts/build.sh <module>`。
6. **ST-Link WinUSB**：Windows Debug 接口；`vendor-pack/STLink/.../USBDriver/`。
7. **两工程**：`f103-manual-reg`（寄存器 + printf/syscalls；**SystemInit** 72 MHz）与 `f103-cmsis-hal`（CubeIDE 对照，非占位；**main** HAL 升频；`HAL_UART_Transmit`）。
8. **PC13 Backup**：`PWREN` + `DBP`（或 `HAL_PWR_EnableBkUpAccess`）后再配 GPIOC。
9. **串口**：**波特率与 COM/tty 名可变**；以固件为准（demo 常 1500000 8N1）；CH341 `1a86:5523` RX←PA9；Win Agent 用 `serial-ch341-read.sh`（勿写死 COM、勿驱动 SecureCRT）；WSL 用 picocom；宿主 `serial-ch341-switch.sh`。
10. **cmsis-hal third_party**：最小子集；HAL Src **9** 个 `.c`；完整上游在 `vendor-pack/`。
11. **fetch 注释**：`copy_hal_minimal` **之后**再 apply（`apply-f103-cmsis-hal-comments.sh`）。
12. **Flash 体积**：manual-reg 纯字符串常 → `puts` ≈**8 KB**；带格式符 ≈30 KB；cmsis-hal ≈6 KB。
13. **注释语言**：`src/`/`startup/`/`linker/` 中文；`third_party/**` 英文；CLI 英文。
14. **Agent 规则**：`AGENTS.md`；`.cursor/rules/*.mdc`（含 `serial-ch341.mdc`）；两份 Skill 同步（`skills/` 为源）。
15. **禁止**未经确认全片 Flash 擦除。
16. **clangd**：`setup-clangd.sh` 同步根 `compile_commands.json`。
17. **调试**：F5「F103 Probe-rs Debug」或「F103 CMSIS-HAL Probe-rs Debug」。
18. **`.gitignore`**：CubeF1/PDF/`.tools/`/`.codegraph/`/`.project-memory-backups/`/`.local/`；CMSIS+HAL submodule 不忽略。
19. **MCP**：`install-mcp-skills.sh`；不覆盖完整 `AGENTS.md`；Codex 无 MCP。
20. **实机**：两工程 build+flash；USART1 高波特经 CH341 验证（Win `serial-ch341-read` / WSL picocom）。
21. **Cursor 终端**：`defaultProfile: Git Bash`。
22. **手册**：DS5319 定方案，RM0008 写寄存器；F103 RCC §6。
23. **VMA/LMA**：`doc/learn/linker-vma-lma.md`。
24. **manual-reg 向量表**：仅 16 项内核异常；扩 IRQ 前须补全。
25. **HAL_DMA_MODULE_ENABLED**：conf 可开但未链 `hal_dma.c`；当前仅阻塞 UART。

## 问题 ↔ 解法

| 问题 | 解法 |
|------|------|
| 找不到文档 / 流程 | `doc/README.md`；日常 `workflow-write-build-flash.md` |
| `clean` 后 `build-flash` 失败 | 先 `./scripts/build.sh <module>` |
| cmsis-hal 缺头 | `fetch-cmsis.sh` → `fetch-f103-cmsis-hal-deps.sh` |
| `probe-rs list` 空 | `stlink-winusb-windows.sh --install` |
| LED 不闪 | PWR+DBP；先 build 再 flash；RESET |
| `--format` 报错 | `--binary-format elf` |
| 无代理 bootstrap 失败 | `--no-proxy` |
| Cursor 找不到工具 | `setup-path.sh`，重启 Cursor |
| clangd 报错 | 先 build，再 `setup-clangd.sh` |
| fetch 后注释变英文 | 重跑 `fetch-f103-cmsis-hal-deps.sh` |
| printf 无输出 | manual-reg 须 `syscalls.c`；cmsis-hal 用 `USART1_WriteStr` |
| 串口右移 | `\n`→`\r\n`（已在 `_write`/`WriteStr`） |
| CH341 无 COM / 无 tty | `serial-ch341-switch.sh to-win` / `to-wsl`（管理员） |
| Win 读串口 / COM 变了 | `./scripts/serial-ch341-read.sh`（自动 VID）；`--list` |
| 乱码 / 波特不对 | 对照固件后 `--baud` / `EMBED_SERIAL_BAUD` |
| WSL picocom | `to-wsl` + `picocom -b <固件波特> /dev/ttyUSB*`；`dialout` |
| 链接缺 `end` | `PROVIDE(end)` + `_sbrk` 用 `_ebss` |
| third_party / 几个 .c | 最小子集；**9** 个 HAL `.c` |
| MCP 调不到 | Connected；非 Codex；新开对话 |
| HSE 失败串口乱码 | BRR 按 72 MHz；HSI 时波特错（已知） |
| uv 无 pyserial | `serial-ch341-read.sh` 内 `uv run --with pyserial` |
