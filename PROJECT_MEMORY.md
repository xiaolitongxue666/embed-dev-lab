# embed-dev-lab — Project Memory

> 项目级持久知识（仅本仓库）。最后更新：2026-06-30（USART1 串口、newlib/nosys 文档）

## 快速路径

| 用途 | 路径 / 命令 |
|------|-------------|
| 一键环境 + 编译 | `./scripts/bootstrap.sh` |
| F103 manual-reg 一键编译+烧录 | `./scripts/build-flash.sh f103-manual-reg` |
| F103 cmsis-hal 一键编译+烧录 | `./scripts/build-flash.sh f103-cmsis-hal` |
| F103 编译 | `./scripts/build.sh <f103-manual-reg\|f103-cmsis-hal> build` |
| F103 烧录 | `./scripts/build.sh <module> flash` |
| f103-cmsis-hal 依赖拷贝 | `./scripts/fetch-f103-cmsis-hal-deps.sh` |
| 应用层文档 | `doc/projects/`（说明）· `projects/`（源码）· `projects/README.md` |
| f103-manual-reg | `doc/projects/f103-manual-reg.md`（全手写寄存器；printf + syscalls 串口） |
| f103-cmsis-hal | `doc/projects/f103-cmsis-hal.md`（CMSIS+HAL；HAL_UART_Transmit，无 printf） |
| 裸机 printf / nosys / HAL 串口 | `doc/learn/newlib-nosys-stdio-retarget.md` |
| f103-cmsis-hal third_party 说明 | `projects/f103-cmsis-hal/third_party/README.md` |
| f103-cmsis-hal 工程结构（带注释目录树） | `projects/f103-cmsis-hal/README.md` |
| 模块编译流程 | `doc/learn/f103-module-build-flow.md` |
| F103 内存映射与启动 | `doc/learn/stm32f103-memory-boot-map.md` |
| F103 MMIO 基础 | `doc/learn/stm32f103-mmio-basics.md` |
| 链接器 map 文件 | `doc/learn/linker-map-file.md` |
| 链接脚本 VMA/LMA | `doc/learn/linker-vma-lma.md` |
| ST 官方文档 fetch | `./scripts/fetch-stm32f103-docs.sh` |
| STM32CubeF1 fetch | `./scripts/fetch-stm32cubef1.sh` |
| ST 官方参考（PDF+MD） | `doc/reference/stm32f103/` |
| CMSIS + HAL submodules | `cmsis-core`（`cm3`/`v5.6.0_cm3`）+ `cmsis-device-f1`（`v4.3.5`）+ `stm32f1xx-hal-driver`（`v1.1.8`）；`./scripts/fetch-cmsis.sh` |
| CMSIS 标准与手写边界 | `doc/learn/cmsis-overview.md` |
| ST F1 软件仓库归纳 | `doc/learn/stm32-cmsis-component-repos.md` |
| STM32CubeF1 参考 | https://github.com/STMicroelectronics/STM32CubeF1 · `vendor-pack/STM32CubeF1/README.md` |
| HAL submodule | `vendor-pack/stm32f1xx-hal-driver` · `v1.1.8` · [`stm32f1xx-hal-driver.embed-dev-lab.md`](vendor-pack/stm32f1xx-hal-driver.embed-dev-lab.md) |
| 中断向量表与 NVIC | `doc/learn/interrupt-vector-table-and-nvic.md` |
| DS5319 / RM0008 分工 | `doc/learn/datasheet-vs-reference-manual.md` |
| 厂商本地资料 | `vendor-pack/`（驱动 / CMSIS submodules / CubeF1 fetch） |
| MCP / Skill 安装 | `./scripts/install-mcp-skills.sh` |
| MCP 校验 | `./scripts/install-mcp-skills.sh --verify-only` |
| ST-Link WinUSB (Windows) | `./scripts/install/stlink-winusb-windows.sh --check-only` |
| 驱动包 | `vendor-pack/STLink/STLink/USBDriver/` |
| 板级 PC13 参考例程 | `vendor-pack/.../核心板测试程序(PC13闪烁)/` |
| IDE F5 调试 | `doc/ide-debug.md`；manual-reg / cmsis-hal 各一套 launch 配置 |

## 编号事实（≤25）

1. **Windows 脚本入口**：Git Bash；`scripts/lib/os-detect.sh` 在非 Windows 拒绝运行需 Git Bash 的逻辑。
2. **PATH**：工具写入 User PATH + `~/.bashrc` 的 `# >>> embed-dev-lab PATH >>>`；IDE 扩展需重启终端或 Cursor 才可见。
3. **代理**：`scripts/lib/proxy.sh` 默认 `http://127.0.0.1:7890`；bootstrap/install-extensions/fetch 脚本/install-mcp-skills 会应用。
4. **主烧录链**：probe-rs（首选）；OpenOCD 可选，`env-check` 中 openocd 为 optional。
5. **probe-rs 烧录 CLI**：`probe-rs download --chip STM32F103C8Tx --binary-format elf <elf>`；旧版 `--format` 已废弃。
6. **烧录后复位**：`scripts/build.sh` 的 `flash` 在 download 后执行 `probe-rs reset`，避免目标停在调试态。
7. **ST-Link WinUSB**：Windows 上 probe-rs 需 Debug 接口 WinUSB；bundled 路径 `vendor-pack/STLink/.../USBDriver/`，脚本 `stlink-winusb-windows.sh`。
8. **projects 布局**：`f103-manual-reg`（全手写寄存器 + `printf`→`syscalls.c`）与 `f103-cmsis-hal`（最小 CMSIS+HAL + `HAL_UART_Transmit`，**无** `syscalls.c`）**并列独立**、行为对等（PC13 闪烁 + 串口打印）；**CMake/build.sh 框架一致**。串口选型见 `doc/learn/newlib-nosys-stdio-retarget.md` §5。
9. **f103-manual-reg 文档链**：`linker-vma-lma.md`（**VMA/LMA 权威**）；`f103-manual-build-from-scratch.md`（从零顺序、GNU ld §2.1–§2.2）；`f103-module-build-flow.md`（CMake/map）；`stm32f103-memory-boot-map.md`（BOOT/Flash/SRAM）；`linker-map-file.md`；链接脚本 `STM32F103C8_FLASH.ld` 行内注释与 startup 协作。
10. **PC13 / Backup 域**：`RCC_APB1ENR.PWREN` + `PWR_CR.DBP` 后再写 `GPIOC_CRH`；详见 `doc/reference/stm32f103/md/topics/backup-domain-pc13.md`。
11. **CMSIS 与本仓库**：`doc/learn/cmsis-overview.md`；submodule `cmsis-core` + `cmsis-device-f1` 供对照；f103-manual-reg **不** `#include` 官方 Device 头。
12. **flash 前需 build**：`build.sh … flash` 不自动编译；`build-flash.sh` 只调 `build` 不调 `configure`。
13. **ST 官方 PDF+MD**：改 `system_stm32f1xx.c` 以 RM0008 §6 RCC 为主；DS5319 定约束；见 `doc/learn/datasheet-vs-reference-manual.md`。
14. **vendor-pack**：ST-Link 驱动 + CMSIS/HAL submodules（Core + Device + HAL）+ 核心板例程 + STM32CubeF1 fetch（可选）。
15. **fetch-cmsis.sh**：Core **`cm3`** / **`v5.6.0_cm3`**，Device **`v4.3.5`**，HAL **`v1.1.8`**；`git submodule update --init` 后 `checkout_hal_driver` 固定 tag；首次 `git clone --recursive`。
16. **f103-cmsis-hal 依赖与 third_party**：`third_party/` **非完整 CMSIS/HAL**，而是 fetch **按需裁剪**的最小子集；完整上游在 `vendor-pack/`；**CMSIS 仅 7 头**；**HAL Inc 全拷、Src 9 个 .c 链入 .elf**（含 `hal_uart.c`）；模块裁剪在 `stm32f1xx_hal_conf.h`（GPIO/RCC/PWR/UART/DMA）；详见 `third_party/README.md`。
17. **用户文档**：`doc/` 中文；应用层 `doc/projects/`；旧链 `doc/modules-f103-blink.md` 为重定向 stub。
18. **USB / ST-Link**：绿联 Hub 下 ST-Link V2 (`0483:3748`) 可正常枚举；SWD 四线。
19. **clangd**：build/bootstrap 后 `setup-clangd.sh` 同步根 `compile_commands.json`（扫描 `projects/*/build/`）。
20. **调试**：Cursor Run →「F103 Probe-rs Debug」（manual-reg）或「F103 CMSIS-HAL Probe-rs Debug」；ELF 路径见 `.vscode/launch.json`。
21. **源码注释与语言**：manual-reg 与 cmsis-hal 的 `src/`、`startup/`、`linker/` 中文；`third_party` ST 正文英文，参与链接的 HAL .c 与 CMSIS 关键头有 embed-dev-lab 中文顶块 + `third_party/**/README.md`；**仅改注释不改代码**；fetch 后 `scripts/lib/apply-f103-cmsis-hal-comments.sh` 自动恢复模板与 third_party 顶注释。
22. **`.gitignore`**：CubeF1 全包、核心板、PDF、`.tools/` 忽略；CMSIS + HAL submodules **不**忽略。
23. **MCP/Skill**：`install-mcp-skills.sh` → `.cursor/mcp.json` + `skills/embed-dev-lab`；Codex 无 MCP。
24. **实机验证**（2026-06-30）：两工程 build+flash 通过；USART1 @ PA9/PA10 **1500000** 8N1；CH341 RX←PA9、GND 共地；`build-flash.sh` 一键可用。
25. **Cursor 终端**：工作区 `.vscode/settings.json` 声明 `defaultProfile: Git Bash`。

## 问题 ↔ 解法

| 问题 | 解法 |
|------|------|
| 找不到工程文档 | `doc/projects/README.md`；源码 `projects/README.md` |
| 从零手写 f103-manual-reg 写什么、顺序 | `doc/learn/f103-manual-build-from-scratch.md`；编译链接见 `f103-module-build-flow.md` |
| `clean` 后 `build-flash` 失败（build 目录不存在） | `build-flash` 不 configure；用 `./scripts/build.sh <module>`（configure+build）再 flash |
| f103-cmsis-hal 一键编译烧录 | `./scripts/build-flash.sh f103-cmsis-hal`（须先 fetch deps；`clean` 后先 `build.sh f103-cmsis-hal`） |
| `probe-rs list` 为空 | Windows：`stlink-winusb-windows.sh --install` 或 Zadig WinUSB |
| 烧录成功但 PC13 不闪 | PWR+DBP；先 `build` 再 `flash`；`probe-rs reset` |
| 程序卡死、无任何 IO | HSE 超时逻辑在 `system_stm32f1xx.c` 的 `SetSysClockTo72()` |
| `unexpected argument '--format'` | 改用 `--binary-format elf` |
| ST PDF curl 超时/SSL 失败 | 浏览器保存至 `pdf/`；`--verify-only` |
| RM0008 topic 页码对不上 | 核对本地 PDF 页脚 Rev |
| 改 system/外设不知看 DS 还是 RM | DS5319 定方案，RM0008 写寄存器；F103 RCC **§6** |
| 手写 startup 算不算用 CMSIS | 未链接官方头文件；遵循 `SystemInit`/向量表即规范兼容 |
| 向量表 / NVIC | f103-manual-reg 仅 16 项内核异常；见 `interrupt-vector-table-and-nvic.md` |
| `0x08000000` vs `0x00000000` | 链接/烧录用 Flash 物理地址；复位读逻辑别名；见 `stm32f103-memory-boot-map.md` |
| System memory | medium-density @ `0x1FFFF000`，12 KB 出厂 ISP ROM |
| 外设地址在 `.data` 吗 / memory map 映 RAM？ | 否；MMIO 写外设硬件；见 `stm32f103-mmio-basics.md` |
| NVIC 和 GPIO 一样吗？ | NVIC 在 PPB `0xE000xxxx`（ARM）；GPIO/RCC 在 `0x40000000`（ST）；见 memory-boot-map §2.1 |
| 链接脚本 / `.ld` 从哪来 | CMSIS `STM32F103XB_FLASH.ld` 改 64K（无 x8 专用模板）；见 `f103-manual-build-from-scratch.md` §3.3 |
| `SECTIONS` / `MEMORY` / VMA·LMA | GNU ld：`> RAM`/`> FLASH`=VMA，`AT>FLASH`=LMA；`.data` 分离、startup 拷贝；权威 **`linker-vma-lma.md`** |
| `ALIGN` / `KEEP` | 见 `f103-manual-build-from-scratch.md` §2.2 |
| `_estack` / 栈「向下增长」 | 满递减栈：`_estack=0x20005000` 为 RAM **上界**；push 时 SP **减小**（非增大）；见 `memory-boot-map` §6.1 |
| 链接命令行 `.obj` 顺序 vs Flash 段布局 | **无关** — 段布局由 `SECTIONS` 决定（如 `.isr_vector` 固定最前）；见 `f103-module-build-flow.md` §3.2 |
| 手册地址与代码 `0x400xxxxx` | 裸机无 MMU，总线实地址 = RM0008；MMIO 见 `stm32f103-mmio-basics.md` |
| clone 后 CMSIS/HAL submodule 空 | `git clone --recursive` 或 `./scripts/fetch-cmsis.sh` |
| pull 后新增 HAL submodule | `git submodule update --init vendor-pack/stm32f1xx-hal-driver` → `./scripts/fetch-cmsis.sh` → `./scripts/fetch-f103-cmsis-hal-deps.sh`；可选 `rm -rf .tools/stm32f1xx-hal-driver-ref` |
| HAL submodule 指针非 v1.1.8 | `fetch-cmsis.sh` 会 checkout `v1.1.8`；`git submodule add` 后须显式 pin tag 再提交指针 |
| GitHub CubeF1 ZIP 缺 CMSIS | `git clone --recursive` 或 `fetch-stm32cubef1.sh` |
| 文档仍写 `modules/` 或 `f103-blink` | 已迁 `projects/f103-manual-reg`；stub 见 `doc/modules-f103-blink.md` |
| bootstrap 慢 | `--build-only --skip-extensions` |
| Cursor 找不到 cmake/clangd | `setup-path.sh`，重启 Cursor |
| IDE clangd 报错 | 先 `build`，再 `setup-clangd.sh` |
| MCP `cargo required` | 安装 Rust，再 `install-mcp-skills.sh` |
| f103-cmsis-hal 缺 third_party / 头文件 | `./scripts/fetch-cmsis.sh` 后 `./scripts/fetch-f103-cmsis-hal-deps.sh` |
| f103-cmsis-hal 链接 assert_param / _init | `stm32f1xx_hal_conf.h` 定义 `assert_param`；startup 无 `__libc_init_array` |
| fetch 后 cmsis-hal 模板注释变英文 | 重新 `./scripts/fetch-f103-cmsis-hal-deps.sh`（apply 恢复 startup/linker/system 与 third_party 顶注释） |
| third_party 是完整 CMSIS/HAL 还是最小子集？ | **最小子集**：CMSIS 7 头 + HAL Inc 全拷 + **9 个 HAL .c 链入 .elf**（含 UART）；完整包在 `vendor-pack/` |
| third_party 是参考还是编入固件？ | **vendored 并入构建**：9 个 HAL `.c` 链入 `.elf`；`Inc/` 仅 `#include`；见 `third_party/README.md` |
| fetch-stm32cubef1 勿用 GitHub ZIP | 缺 submodule；用 `git clone --recursive` 或 `fetch-stm32cubef1.sh` |
| Cursor Agent 调不到 MCP | 确认 MCP 已连接；非 Codex；新开对话 |
| printf 无串口 / `_write is not implemented` | 须 `USART1_Init`/`MX_USART1_UART_Init`；**manual-reg** 须 `syscalls.c` 实现 `_write`→`USART1_Write`；**cmsis-hal** 用 `USART1_WriteStr`→`HAL_UART_Transmit`（无 printf） |
| 串口有输出但逐行右移 | Windows 终端需 CRLF；`_write`/`USART1_WriteStr` 在 `\n` 前补 `\r` |
| 链接 `undefined reference to 'end'` | manual-reg 链接脚本 `PROVIDE(end)`；`syscalls.c` 自定义 `_sbrk` 用 `_ebss` |
| HAL 也要 syscalls / `_write` 吗 | 仅 **printf** 需要；HAL 只管发字节；概念见 `doc/learn/newlib-nosys-stdio-retarget.md` |
| `--specs=nosys.specs` 必须写 syscalls？ | 否；**用 printf 才须 `_write`**；不用 libc I/O 时 libnosys 占位即可链接 |
| third_party HAL .c 个数 | **9 个**（含 `hal_uart.c`）；见 `f103-cmsis-hal/CMakeLists.txt` |
