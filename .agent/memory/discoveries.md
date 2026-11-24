# 探索发现

引脚、时序和器件事实。已定为工程约定的行为见 [`decisions.md`](decisions.md)。

- PC13：Backup 域，灌电流 LED，低电平亮，不是 FT。PB12：FT，拉电流 LED，220 Ω，与 PC13 同电平做异或。PB13 KEY：FT 上拉，按下为低；用 `PBin` 读 IDR。`PBout` / `PBin` 在 `gpioc_bitband.h`。
- USART1：PA9/PA10 为 FT，1500000 8N1，`BRR=0x30`。DMA1 CH4 TX、CH5 RX。不要把 PA13/PA14 当 UART。`GPIOA_CRH` 里 PA9 半字节 `0xB` 是复用推挽。
- `f103-manual-rtos` 状态任务的 `LOG_D`（`vsnprintf` 160 字节缓冲再加一次 `printf`）在任务栈上约用 1028 字节。1024 字节会写进下一块任务栈。
- CH341：VID:PID `1a86:5523`。RX←PA9，TX→PA10，GND 进面包板地。
- SPI1 共用：PA5 SCK、PA7 MOSI、PA6 MISO。软件 CS：PA3=BMP280 Mode0，PA8=LSM6 Mode3。
- BMP280：id 寄存器 `0xD0` 为 `0x58`（BME 为 `0x60`）。校准 24 字节在 `0x88`。先写 `0xF5=0x24`，再写 `0xF4=0x57`。先 `ReadTemp` 再 `ReadPressure`。`bmp280_delay_ms` 用 `SysTick_GetMs`（单调）。实现在 `bmp280.c`。
- LSM6DS3：Mode3，CS←PA8，`WHO_AM_I=0x69`。
- 向量表按完整 F103xB。USART1 IRQn=37。DMA1 CH4/CH5/CH6 = 14/15/16。`systick.c` 里的 `SysTick_Handler` 覆盖 weak `Default`。
- PA0：旋钮，ADC，DMA1 CH1。PA1：TIM2_CH2，24 kHz 风扇，死区 80。
- 日志里 `LED on|off` 是 GPIO 电平，不是「灯亮/灯灭」的口语。`KEY high|low` 是 IDR。
