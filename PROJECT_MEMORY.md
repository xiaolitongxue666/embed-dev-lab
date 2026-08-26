# embed-dev-lab — Project Memory

> 项目级持久知识（仅本仓库）。最后更新：2026-08-26（summary-memory：文档/规则审查收口）

## 快速路径

| 用途 | 路径 / 命令 |
|------|-------------|
| 编写 → 编译 → 下载 | `doc/workflow-write-build-flash.md` |
| Agent 规则 / Skill | `AGENTS.md` · `skills/embed-dev-lab/SKILL.md` · `.cursor/rules/` |
| 一键环境 | `./scripts/bootstrap.sh`（无代理 `--no-proxy`） |
| 编译 / 烧录 | `./scripts/build.sh <module> build\|flash`；一键 `build-flash.sh`（不 configure） |
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
4. **烧录**：probe-rs 主路径；chip `STM32F103C8Tx`；`--binary-format elf`；flash 后 `probe-rs reset`。
5. **flash 不 compile / 不 configure**：改代码先 `build`；`clean` 后先 `./scripts/build.sh <module>`；流程 `doc/workflow-write-build-flash.md`。
6. **ST-Link WinUSB**：Windows Debug 接口；`vendor-pack/STLink/.../USBDriver/`。
7. **两工程并列**：`f103-manual-reg`（手写寄存器 + printf/syscalls；**SystemInit** 升 72 MHz）与 `f103-cmsis-hal`（CubeIDE 风格对照，非占位/非 CubeMX；**main** 内 HAL 升频；`HAL_UART_Transmit`，无 printf）。
8. **PC13 Backup**：`PWREN` + `DBP`（或 `HAL_PWR_EnableBkUpAccess`）后再配 GPIOC。
9. **串口**：USART1 PA9/PA10，**1500000** 8N1；CH341 RX←PA9、GND；`\n` 补 `\r`。
10. **cmsis-hal third_party**：最小子集；CMSIS 7 头；HAL Inc 全拷、**Src 9 个 .c**（含 `hal_uart.c`）；完整上游在 `vendor-pack/`。
11. **fetch 注释**：`copy_hal_minimal` **之后**再 `apply_f103_third_party_embed_notes`（否则 HAL 顶注释被覆盖）；脚本 `scripts/lib/apply-f103-cmsis-hal-comments.sh`。
12. **Flash 体积**：manual-reg 纯字符串 `printf` 常优化为 `puts`，Debug ≈**8 KB** text；带格式符才接近 30 KB；cmsis-hal ≈6 KB。
13. **注释语言**：`src/`/`startup/`/`linker/` 中文；`third_party/**` vendor 英文；CLI 英文。
14. **Agent 规则**：`AGENTS.md` SSOT 摘要；`.cursor/rules/*.mdc` 按 glob；两份 Skill 须同步（`skills/` 为源）。
15. **禁止**未经确认全片 Flash 擦除。
16. **clangd**：`setup-clangd.sh` 同步根 `compile_commands.json`。
17. **调试**：F5「F103 Probe-rs Debug」或「F103 CMSIS-HAL Probe-rs Debug」。
18. **`.gitignore`**：CubeF1/PDF/`.tools/`/`.codegraph/`/`.project-memory-backups/`；CMSIS+HAL submodule 不忽略。
19. **MCP**：`install-mcp-skills.sh`；不覆盖已有完整 `AGENTS.md`；Codex 无 MCP。
20. **实机**（2026-06-30）：两工程 build+flash；USART1 1500000 验证通过。
21. **Cursor 终端**：`defaultProfile: Git Bash`。
22. **手册分工**：DS5319 定方案，RM0008 写寄存器；F103 RCC §6。
23. **VMA/LMA 权威**：`doc/learn/linker-vma-lma.md`。
24. **manual-reg 向量表**：仅 16 项内核异常；扩展外设 IRQ 前须补全。
25. **HAL_DMA_MODULE_ENABLED**：conf 可开但未链 `hal_dma.c`；当前仅阻塞 UART。

## 问题 ↔ 解法

| 问题 | 解法 |
|------|------|
| 找不到文档 / 流程 | `doc/README.md`；日常 `workflow-write-build-flash.md` |
| `clean` 后 `build-flash` 失败 | 先 `./scripts/build.sh <module>`（configure+build） |
| cmsis-hal 缺头 / 首次构建 | `fetch-cmsis.sh` → `fetch-f103-cmsis-hal-deps.sh` |
| `probe-rs list` 空 | `stlink-winusb-windows.sh --install` |
| LED 不闪 | PWR+DBP；先 build 再 flash；RESET |
| `--format` 报错 | 改用 `--binary-format elf` |
| 无代理 bootstrap 慢/失败 | `--no-proxy` |
| Cursor 找不到工具 | `setup-path.sh`，重启 Cursor |
| clangd 报错 | 先 build，再 `setup-clangd.sh` |
| fetch 后注释变英文 | 重跑 `fetch-f103-cmsis-hal-deps.sh`（含 apply） |
| printf 无输出 / `_write` | manual-reg 须 `syscalls.c`；cmsis-hal 用 `USART1_WriteStr` |
| 串口右移 | `\n`→`\r\n`（已在 `_write`/`WriteStr`） |
| 链接缺 `end` | `PROVIDE(end)` + `_sbrk` 用 `_ebss` |
| third_party 完整吗 / 几个 .c | **最小子集**；**9** 个 HAL `.c`（含 UART） |
| 旧文档写「占位」/「8 个」 | 已纠正为 CubeIDE 对照 / 9 个；见 `AGENTS.md` |
| MCP 调不到 | 确认 Connected；非 Codex；新开对话 |
| HSE 失败串口乱码 | BRR 仍按 72 MHz；保持 HSI 时波特率错误（已知，未改固件） |
