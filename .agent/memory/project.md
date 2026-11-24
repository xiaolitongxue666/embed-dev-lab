# 项目背景

来源：原 [`PROJECT_MEMORY.md`](../../PROJECT_MEMORY.md) 的环境与文档条目。技术选择见 [`decisions.md`](decisions.md)。

- Windows：脚本在 Git Bash 运行（`os-detect.sh`）。Cursor `defaultProfile` 为 Git Bash。
- 仓库是 STM32F103 双路线实验：`f103-manual-reg` 与 `f103-cmsis-hal`。对齐规则见 [`../rules/architecture.md`](../rules/architecture.md)。`f103-manual-rtos` 是手写调度学习工程，不在这条对齐里。
- clangd：`--target=arm-none-eabi`；`settings.local.json` 用 query-driver overlay。
- 脚本默认 HTTP 代理 `http://127.0.0.1:7890`。`serial-ch341-read.sh` 走 `uv --with pyserial` 时也需要该代理。
- 文档：`doc/hardware/`（外设、引脚、供电）、`doc/reference/bmp280/`、`doc/reference/sh1106/`、`doc/reference/stm32f103/md/topics/i2c1-master-polling.md`、`dma1-ahb-clock.md`、`doc/workflow-write-build-flash.md`。
- `.gitignore` 包含：STM32CubeF1 大包、PDF dump、`.tools`、`.codegraph`、`.project-memory-backups`。
