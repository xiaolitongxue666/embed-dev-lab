# Project Memory (Compact)

1) Windows: scripts in Git Bash (os-detect.sh); Cursor defaultProfile=Git Bash.
2) Build/flash: ./scripts/build.sh <module> build|flash; flash does not compile; clean then build first; chip STM32F103C8Tx; --binary-format elf.
3) Dual tracks: f103-manual-reg (bare regs) vs f103-cmsis-hal (CMSIS+HAL Cube-style). Align external behavior unless user scopes one track. 2026-09-21 user scoped **manual-reg only** for software timers / four-field OLED / LOG_* (cmsis-hal still HAL_GetTick 1s HH:MM:SS, no log module).
4) PC13 Backup: PWREN+DBP before GPIOC_CRH; sink LED low=on; not FT. PB12: FT; source LED 220Ω; XOR same level as PC13. PB13 KEY: FT pull-up, low=pressed; read IDR via PBin. PBout/PBin in gpioc_bitband.h.
5) USART1: PA9/PA10 FT; 1500000 8N1; DMA1 CH4 TX + CH5 RX + IDLEIE; HTIE=0 TCIE only; printf→syscalls→USART1_Write (blocks on g_usart1_tx_busy). RX handle=USART1_EchoFrame. CH341 1a86:5523 RX←PA9 TX→PA10 GND. Agent: serial-ch341-read.sh（uv --with pyserial 须代理 127.0.0.1:7890）。Open serial then reset for boot lines. Never PA13/PA14 as UART.
6) Startup: g_pfnVectors → Reset_Handler → SystemInit (HSE×PLL→72MHz) → main; -nostartfiles. HSE fail → HSI 8MHz (UART/I2C/SysTick all slow/wrong).
7) SPI1 shared: PA5 SCK / PA7 MOSI / PA6 MISO; software CS PA3=BMP280 Mode0, PA8=LSM6 Mode3. TransferByte timeout→0xFF; 0x00=MISO stuck low / CS not selecting.
8) BMP280: CSB←PA3 (PA4 hole dead). SDO→PA6 never GND. id 0xD0: 0x58 / BME 0x60. Calib 24 B @0x88; 0xF5=0x24 then 0xF4=0x57. ReadTemp before ReadPressure. bmp280_delay_ms uses SysTick_GetMs (monotonic). Source: bmp280.c.
9) LSM6DS3: Mode3; CS←PA8; WHO_AM_I=0x69. Hardware not on bench yet (2026-09-16). main does not call it.
10) I2C1: PB6/PB7 AF_OD; 400kHz CCR=30 TRISE=11. SH1106 write addr 0x78 raw (never <<1). Page = I2C1_WriteDma DMA1 CH6 128B. Full Refresh ≈20ms+; do not refresh OLED every 10ms.
11) SH1106 (manual-reg 2026-09-21): DrawClock(h,m,s,centi) draws `00:00:00:00` at 8×16 (11 glyphs, x=20 y=24); 2x HH:MM:SS does not fit 128 cols. Temp still y=48. Refresh 50ms (TIMER_EVT_OLED_CLOCK); temp cached, BMP280 read on 1s slot. SetMs(0) after Init resets monotonic + ms/sec together.
12) SysTick (manual-reg): keep HCLK / 71999, not HCLK/8 LOAD=9000, no CMSIS. ISR: s_systick_ms++ (GetMs), s_sys_ms++/sec++, TimerEvent_OnTick(). SysTime_Get retries on **sec** (not ms) to avoid wrap tear. irq lock = mrs PRIMASK + cpsid / restore.
13) timer_event lives in **periph** (ISR must not call app). 4 slots; period_ms==0 skip; TakeFlag read-clear. OLED 50ms periodic; LED/log 1000ms periodic. main has no delay(0x7FFF)/led_phase.
14) LOG_* (app/log.c): `[HH:MM:SS][I]` second-only; E red / W yel / I grn / D cyan ANSI; LOG_COMPILE_LEVEL default DEBUG; LOG_COLOR=1. One vsnprintf + one printf per line (3×printf caused UART stutter). 1s DEBUG is **one** line (LED/KEY/knob/clock[/temp]). Never log in ISR. SecureCRT needs ANSI Color.
15) Docs: hardware/peripherals+pinout+power; reference/bmp280/ sh1106/; i2c1-master-polling.md; dma1-ahb-clock.md; workflow-write-build-flash.md. .gitignore: CubeF1; pdf dumps; .tools; .codegraph; .project-memory-backups.
16) cmsis-hal: fetch-cmsis.sh then fetch-f103-cmsis-hal-deps.sh; HAL Src **16** .c; comments Chinese in src/startup/linker; CLI English. Clock in main SystemClock_Config.
17) Never mass-erase Flash without confirm. Unpowered MCU → JtagGetIdcodeError; probe-rs list empty = ST-Link USB gone.
18) Vector table full F103xB. USART1 IRQn=37; DMA1 CH4/5/6 =14/15/16. SysTick_Handler in systick.c overrides weak Default.
19) clangd: --target=arm-none-eabi; settings.local.json query-driver overlay. Power: board MicroUSB→MCU; ST-Link 3.3V→breadboard (scheme A 5V idle); star GND. Proxy 127.0.0.1:7890.
20) Pinout: PA0 knob ADC DMA1 CH1; PA1 TIM2_CH2 24 kHz fan, deadzone 80. Logs: LED on|off=GPIO level; KEY high|low=IDR.
21) 2026-09-21 flash OK COM8 1500000: `[00:00:01][D] LED on  KEY high  knob raw=…  clock 00:00:01:00` ~1s. OLED four-field running. BMP280 temp often absent this bench (init fail → 0.00C).
