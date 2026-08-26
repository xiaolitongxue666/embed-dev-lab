## embed-dev-lab Skill (Codex / 无 MCP Agent)

- **f103-manual-reg**：全手写寄存器，不链接 CMSIS/HAL；串口用 `printf` + `syscalls.c` → `_write`；时钟在 `SystemInit`（进 `main` 前）升至 72 MHz
- **f103-cmsis-hal**：CubeIDE 风格对照（`MX_*` / MSP / `hal_conf`），**非**占位、**非** CubeMX 生成工程；工程内 CMSIS+HAL 最小子集（HAL Src **9** 个 `.c`）；串口用 `HAL_UART_Transmit`（无 printf/syscalls）；时钟在 `main` 的 `SystemClock_Config`
- 构建：`./scripts/build.sh <module> build`；烧录：`flash`（**不**自动 compile / configure；改代码后须先 build；`clean` 后须先 `build.sh <module>` 再 flash）
- cmsis-hal 依赖：`./scripts/fetch-f103-cmsis-hal-deps.sh`（须先 `fetch-cmsis.sh`）
- probe-rs chip：`STM32F103C8Tx`；CLI 使用 `--binary-format elf`
- PC13 属于 Backup 域：配置前须 `RCC_APB1ENR.PWREN` + `PWR_CR.DBP`（manual-reg）或 HAL `HAL_PWR_EnableBkUpAccess()`（cmsis-hal）
- 禁止未经确认的全片 Flash 擦除
- CMSIS + HAL submodule：`cmsis-core`（`cm3`/`v5.6.0_cm3`）+ `cmsis-device-f1`（`v4.3.5`）+ `stm32f1xx-hal-driver`（`v1.1.8`）；`./scripts/fetch-cmsis.sh`
- 注释：`src/`、`startup/`、`linker/` 中文；`third_party/**` vendor 英文；fetch 后由 `scripts/lib/apply-f103-cmsis-hal-comments.sh` 恢复模板中文注释
- CH341 串口宿主：`./scripts/serial-ch341-switch.sh status|to-win|to-wsl`
- Windows Agent 读串口：`./scripts/serial-ch341-read.sh [--baud N]`（自动找 CH341 COM；波特/端口可变）
- WSL：`picocom -b <固件波特> /dev/ttyUSB*`
- 规则：`.cursor/rules/serial-ch341.mdc`；详解：`doc/scripts-reference.md`
- 流程：`doc/workflow-write-build-flash.md`（烧写入口：`scripts/build.sh` / `build-flash.sh`）；详见 `skills/embed-dev-lab/SKILL.md` 与 `doc/probe-rs.md`
