# 问题与处理

已经碰到过的失败和对应处理。

| 现象 | 处理 |
|------|------|
| HSE 失败 | 落到 HSI 8 MHz。UART、I2C、SysTick 都会变慢或错。启动链是 `g_pfnVectors` → `Reset_Handler` → `SystemInit`（HSE×PLL→72 MHz）→ `main`，`-nostartfiles` |
| 未上电的 MCU | `JtagGetIdcodeError`。`probe-rs list` 为空表示 ST-Link USB 不在 |
| 想看启动日志却错过 | 先打开串口，再复位板子 |
| SPI 读到 `0x00` | MISO 被拉死或 CS 没有选中该器件。`TransferByte` 超时返回 `0xFF` |
| BMP280 片选无效 | PA4 孔是坏的。CSB 接 PA3。SDO 接 PA6，不要接 GND |
| 这块台架经常没有 BMP280 温度 | 初始化失败时 OLED 画 `0.00C` |
| OLED 刷新过频 | 整屏刷新约 20 ms 以上，不要每 10 ms 刷新 |

未经用户确认，不要用全片擦除来「修」烧录失败。
