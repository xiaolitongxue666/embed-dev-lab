# Project Memory (Compact)

1) Windows: scripts in Git Bash (os-detect.sh); Cursor defaultProfile=Git Bash.
2) Build/flash: ./scripts/build.sh <module> build|flash; flash does not compile; clean then build first; chip STM32F103C8Tx; --binary-format elf.
3) Dual tracks: f103-manual-reg (bare regs) vs f103-cmsis-hal (CMSIS+HAL Cube-style). Align external behavior unless user scopes one track. SPI/LSM6DS3, I2C1/SH1106, PB12 LED, PB13 KEY, USART DMA+IDLE, ADC1 PA0 are manual-reg only (2026-09); cmsis-hal still blocking TX-only.
4) PC13 Backup: PWREN+DBP before GPIOC_CRH; sink LED low=on; not FT. PB12: FT; source LED 220Ω; XOR same level as PC13. PB13 KEY: FT pull-up, low=pressed; read IDR via PBin. PBout/PBin in gpioc_bitband.h.
5) USART1: PA9/PA10 FT; 1500000 8N1; DMA1 CH4 TX + CH5 RX + IDLEIE; HTIE=0 TCIE only; printf→syscalls→USART1_Write. CH341 1a86:5523 RX←PA9 TX→PA10 GND. Agent: serial-ch341-read.sh. Open serial then reset for boot lines. Never PA13/PA14 as UART.
6) Startup: g_pfnVectors → Reset_Handler → SystemInit (HSE×PLL→72MHz) → main; -nostartfiles. HSE fail → HSI 8MHz (UART/I2C/SysTick all slow/wrong).
7) LSM6DS3: HW SPI1 Mode3; silk SCL←PA5 SDA←PA7 SAO→PA6 CS←PA4; WHO_AM_I=0x69 (0x00/0xFF=unwired OK). SPI1 IMU-only; no W25Q/TF as SPI lesson.
8) I2C1: PB6 SCL / PB7 SDA AF_OD 0xF; 400kHz PCLK1=36MHz CCR=30 TRISE=11; OAR1 bit14=1. Addr param is 8-bit write; SH1106=0x78 raw to DR (never <<1). StdPeriph Send7bitAddress(0x78>>1) must not be copied. Probe timeout+AF→STOP. Burst page write (0x40+128), not per-byte START/STOP.
9) SH1106: 128×64 vis / 132×64 GDDRAM; ctrl 0x00 cmd / 0x40 data; charge pump 0x8D/0x14; each page 0xB0+n, 0x02, 0x10. API: Init/Clear/DrawPixel/DrawClock/Refresh. 4-pin no RES (busy wait). VCC breadboard 3.3V; module pull-ups. Demo: centered HH:MM:SS (8×16×2). SysTick 1ms software clock (not RTC); SetMs(0) after Init so display starts 00:00:00. Logs: SH1106 ACK addr=0x78 / NACK / clock HH:MM:SS.
10) Docs: hardware/peripherals+pinout+power; reference/sh1106/; i2c1-master-polling.md; lsm6ds3/; workflow-write-build-flash.md. .gitignore: CubeF1; stm32f103/lsm6ds3/sh1106 pdf; .tools; .codegraph; .project-memory-backups.
11) cmsis-hal: fetch-cmsis.sh then fetch-f103-cmsis-hal-deps.sh; HAL Src 9 .c; comments Chinese in src/startup/linker; CLI English.
12) Never mass-erase Flash without confirm. Unpowered MCU → JtagGetIdcodeError; probe-rs list empty = ST-Link USB gone.
13) Vector table full F103xB. USART1 IRQn=37; DMA1 CH4/5 =14/15. SysTick_Handler in systick.c overrides weak Default. Wrong IRQ name → Default_Handler.
14) clangd: --target=arm-none-eabi; settings.local.json query-driver overlay. Do not change usart DMA pointer casts.
15) Power: board MicroUSB→MCU; ST-Link 3.3V→breadboard (scheme A 5V idle); star GND on breadboard rail. Proxy 127.0.0.1:7890.
16) Pinout/FT: PA0–PA7, PB0/PB1, PB5, PC13–15 not FT; PB6/PB7 FT. ADC only ADC12_INx. No USART2. Knob SIG→PA0. Logs: LED on|off=GPIO level; KEY high|low=IDR; knob raw/mv.
