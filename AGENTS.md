
## embed-dev-lab Skill (Codex / 无 MCP Agent)

- 构建：`./scripts/build.sh f103-blink build`；烧录：`flash`（需先 build）
- probe-rs chip：`STM32F103C8Tx`；CLI 使用 `--binary-format elf`
- PC13 属于 Backup 域：配置前须 `RCC_APB1ENR.PWREN` + `PWR_CR.DBP`
- 禁止未经确认的全片 Flash 擦除
- CMSIS submodule：`cmsis-core`（`cm3`/`v5.6.0_cm3`）+ `cmsis-device-f1`（`v4.3.5`）；`./scripts/fetch-cmsis.sh`
- 应用文档：`doc/modules/`（源码在 `modules/`）
- 详见 `skills/embed-dev-lab/SKILL.md` 与 `doc/probe-rs.md`
