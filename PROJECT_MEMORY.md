# Project Memory (Compact)

1) Windows: scripts must run in Git Bash (os-detect.sh); Cursor defaultProfile=Git Bash.
2) Build/flash: ./scripts/build.sh <module> build|flash; flash does not compile/configure; clean then build first; chip STM32F103C8Tx; --binary-format elf.
3) Dual tracks: f103-manual-reg (bare regs) vs f103-cmsis-hal (CMSIS+HAL Cube-style); align external behavior. SPI/LSM6DS3 (2026-08) is manual-reg only; cmsis-hal not synced yet.
4) PC13 Backup: PWREN+DBP (or HAL_PWR_EnableBkUpAccess) before GPIOC_CRH.
5) USART1: PA9 TX / PA10 RX; baud/COM vary (demo often 1500000 8N1); CH341 1a86:5523; serial-ch341-read.sh / switch.sh; open serial then probe-rs reset to catch boot lines.
6) Startup chain: g_pfnVectors -> Reset_Handler (startup_stm32f103xb.s: SP, .data, .bss) -> bl SystemInit (system_stm32f1xx.c HSE×PLL→72MHz) -> bl main; -nostartfiles no crt0; docs in f103-manual-reg.md and stm32-bare-metal-bootstrap.md Q11.
7) LSM6DS3: hardware SPI1 (not GPIO soft/bitbang); CR1/SR/DR drive SCK/MOSI/MISO; CS=GPIO PA4; Mode 3; silk SCL<-PA5 SDA<-PA7 SAO->PA6 CS<-PA4; 3V3 not VIN; WHO_AM_I=0x69 (TR-C 0x6A); solder min 6 pads (INT/OCS/SCX/SDX optional); 104Hz ±2g/250dps; ≥20ms after power.
8) Soft SPI vs HW SPI + signal≠register names: documented in spi.c/spi.h headers and f103-manual-reg.md; board still 4 wires.
9) Doc paths: hardware/stm32f103-peripherals.md (wiring+solder note); reference/lsm6ds3/; reference/stm32f103/; workflow-write-build-flash.md.
10) SPI1 reserved for IMU only; display on I2C; do not use W25Q/TF as SPI learning peripheral.
11) cmsis-hal: fetch-cmsis.sh then fetch-f103-cmsis-hal-deps.sh; HAL Src 9 .c; apply-f103-cmsis-hal-comments.sh after copy.
12) Comments: src/startup/linker Chinese; third_party English; CLI English.
13) .gitignore: CubeF1; stm32f103+lsm6ds3 pdf; .tools; .codegraph; .project-memory-backups; keep serial-ch341-*.sh and CMSIS/HAL submodules.
14) Never mass-erase Flash without explicit user confirm.
15) Agent rules: AGENTS.md; .cursor/rules (serial-ch341.mdc); skills/embed-dev-lab SKILL.md.
16) Flash size: manual-reg puts~8KB / printf-format~30KB; cmsis-hal~6KB.
17) HSE fail keeps HSI 8MHz; USART BRR assumes 72MHz → garbled UART if HSE fails (known).
18) Vector table manual-reg: 16 core exceptions only; extend IRQs before enabling peripherals.
19) clangd: setup-clangd.sh syncs root compile_commands.json after build. MCP: install-mcp-skills.sh; Codex has no MCP.
20) Proxy default http://127.0.0.1:7890; --no-proxy to disable.
21) ST-Link WinUSB: stlink-winusb-windows.sh; vendor-pack/STLink USBDriver.
22) WHO_AM_I 0x00/0xFF: check 3V3/GND/CS/SCL/SDA/SAO; expected if IMU unwired. 0x6A means TR-C.
23) Docs entry: doc/README.md; probe-rs.md; scripts-reference.md; getting-started.md.
24) HAL_DMA_MODULE_ENABLED may be on without linking hal_dma.c; UART is blocking only.
25) VMA/LMA: doc/learn/linker-vma-lma.md; DS5319 for pin plan, RM0008 for registers.
