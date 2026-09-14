# Project Memory (Compact)

1) Windows: scripts must run in Git Bash (os-detect.sh); Cursor defaultProfile=Git Bash.
2) Build/flash: ./scripts/build.sh <module> build|flash; flash does not compile/configure; clean then build first; chip STM32F103C8Tx; --binary-format elf.
3) Dual tracks: f103-manual-reg (bare regs) vs f103-cmsis-hal (CMSIS+HAL Cube-style); align external behavior unless user scopes one track. SPI/LSM6DS3, PB12 LED, PB13 KEY, USART IT bidirectional (2026-09) are manual-reg only; cmsis-hal still blocking TX-only.
4) PC13 Backup: PWREN+DBP before GPIOC_CRH; sink LED low=on; not FT. PB12: FT; IOPBEN + CRH 50MHz PP (bits 19:16); source LED GPIO→220Ω→LED+→GND; XOR same level as PC13 → complementary blink. PB13 KEY: FT; CRH CNF=10 MODE=00 + ODR=1 pull-up; PB13←key→GND; low=pressed; read IDR via PBin (not ODR). PBout/PBin in gpioc_bitband.h.
5) USART1: PA9 TX / PA10 RX (FT); 1500000 8N1 typical. manual-reg: TX/RX IRQ + global rings (TX 256/RX 128); USART1_Write enqueues; USART1_IRQHandler only moves rings/clears ORE; RX handle runs in USART1_ProcessRx (main), not ISR; demo echoes each RX byte. printf still syscalls→USART1_Write. CH341 1a86:5523 RX←PA9 TX→PA10 GND. Agent: serial-ch341-read.sh [--send TEXT|hex:..] / switch.sh; GUI: 1500000 8N1, disable local echo; typed echo interleaves with LED/KEY lines. Open serial then reset to catch WHO_AM_I. Never PA13/PA14 as UART. COM PermissionError → SecureCRT holding port. bytes=0 with LEDs blinking: breadboard/TX-RX swap first.
6) Startup chain: g_pfnVectors -> Reset_Handler (startup_stm32f103xb.s: SP, .data, .bss) -> bl SystemInit (system_stm32f1xx.c HSE×PLL→72MHz) -> bl main; -nostartfiles no crt0; docs in f103-manual-reg.md and stm32-bare-metal-bootstrap.md Q11.
7) LSM6DS3: hardware SPI1 (not GPIO soft/bitbang); CR1/SR/DR drive SCK/MOSI/MISO; CS=GPIO PA4; Mode 3; silk SCL<-PA5 SDA<-PA7 SAO->PA6 CS<-PA4; PA4–PA7 not FT; power from breadboard 3.3V (ST-Link), not VIN/board rail; WHO_AM_I=0x69 (TR-C 0x6A); 0x00/0xFF→check wiring (ok if unwired); solder min 6 pads; ≥20ms after power. Soft vs HW SPI + signal≠register names in spi.c/spi.h.
8) Doc paths: hardware/power-and-common-ground.md (power/GND); hardware/stm32f103-peripherals.md (wiring); hardware/stm32f103c8t6-pinout.md (silk+FT+occupancy); reference/stm32f103/md/topics/lqfp48-pinout.md (DS5319 Table 5); reference/lsm6ds3/; workflow-write-build-flash.md.
9) SPI1 reserved for IMU only; display on I2C; do not use W25Q/TF as SPI learning peripheral.
10) cmsis-hal: fetch-cmsis.sh then fetch-f103-cmsis-hal-deps.sh; HAL Src 9 .c; apply-f103-cmsis-hal-comments.sh after copy.
11) Comments: src/startup/linker Chinese; third_party English; CLI English.
12) .gitignore: CubeF1; stm32f103+lsm6ds3 pdf; .tools; .codegraph; .project-memory-backups; keep serial-ch341-*.sh and CMSIS/HAL submodules.
13) Never mass-erase Flash without explicit user confirm.
14) Agent rules: AGENTS.md; .cursor/rules (serial-ch341.mdc, hardware-power.mdc); skills/embed-dev-lab SKILL.md.
15) Flash size: manual-reg puts~8KB / printf-format~30KB; cmsis-hal~6KB.
16) HSE fail keeps HSI 8MHz; USART BRR assumes 72MHz → garbled UART if HSE fails (known).
17) Vector table manual-reg is full F103xB (core 0–15 + peripherals through USBWakeUp/BootRAM). USART1 is vector index 53, IRQn=37; nvic.c writes ISER/IP (no core_cm3.h). Wrong IRQHandler name silently hits Default_Handler.
18) clangd: setup-clangd.sh syncs root compile_commands.json after build. MCP: install-mcp-skills.sh; Codex has no MCP.
19) Proxy default http://127.0.0.1:7890; --no-proxy to disable.
20) Power/SWD: board MicroUSB powers MCU; ST-Link SWD + 3.3V→breadboard (scheme A: 5V idle; never board VDD); star GND on breadboard rail (ST-Link/CH341/board SWD/peripherals; no series ground). Unpowered MCU → probe-rs JtagGetIdcodeError even if ST-Link lists. WinUSB via stlink-winusb-windows.sh; doc/hardware/power-and-common-ground.md + hardware-power.mdc.
21) Docs entry: doc/README.md; probe-rs.md; scripts-reference.md; getting-started.md. Learn: gpio-eight-modes / gpio-protection / gpio-led-source-sink / uart-ttl-rs232-rs485 / swd-vs-usart; gpio-cnf-mode.md. F103 GPIO ±8 mA (relaxed ±20); PC13–15 ±3 mA. Logs: PC13/PB12 LED on|off = GPIO level; PB13 KEY high|low = IDR (low=pressed).
22) HAL_DMA_MODULE_ENABLED may be on without linking hal_dma.c; UART is blocking only.
23) Pinout/FT (DS5319 Table 5): PA0–PA7, PB0/PB1, PB5, PC13–15 not FT; PA8–15 and PB2–4/PB6–15 are FT among common GPIOs; ADC channels = ADC12_INx; do not trust web “all GPIO 5V-tolerant” tables; USART2/3 remap pins absent on LQFP48. VMA/LMA: linker-vma-lma.md; DS5319 pin plan, RM0008 registers.
