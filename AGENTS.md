
## embed-dev-lab Skill (Codex / 无 MCP Agent)

- **f103-manual-reg**：全手写寄存器，不链接 CMSIS/HAL
- **f103-cmsis-hal**：工程内 CMSIS+HAL 最小子集，CMake 框架与 manual-reg 一致
- 构建：`./scripts/build.sh <module> build`；烧录：`flash`（需先 build）
- cmsis-hal 依赖：`./scripts/fetch-f103-cmsis-hal-deps.sh`（须先 `fetch-cmsis.sh`）
- probe-rs chip：`STM32F103C8Tx`；CLI 使用 `--binary-format elf`
- PC13 属于 Backup 域：配置前须 `RCC_APB1ENR.PWREN` + `PWR_CR.DBP`（manual-reg）或 HAL `HAL_PWR_EnableBkUpAccess()`（cmsis-hal）
- 禁止未经确认的全片 Flash 擦除
- CMSIS submodule：`cmsis-core`（`cm3`/`v5.6.0_cm3`）+ `cmsis-device-f1`（`v4.3.5`）；`./scripts/fetch-cmsis.sh`
- 注释：`src/`、`startup/`、`linker/` 中文；`third_party/**` vendor 英文；fetch 后自动恢复模板中文注释
- 详见 `skills/embed-dev-lab/SKILL.md` 与 `doc/probe-rs.md`
