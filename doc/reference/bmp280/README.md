# BMP280 / BME280 SPI

Bosch 气压/温度传感器，本仓库走 **SPI1 4 线**，与 LSM6DS3 共用 PA5/PA7/PA6，片选独立。

| 项 | 值 |
|----|-----|
| 手册 | [BST-BMP280-DS001](https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bmp280-ds001.pdf) |
| 接口 | 4 线 SPI（CSB 拉低选 SPI；CSB 悬空/拉高走 I2C） |
| 模式 | Mode **00**（CPOL=0，CPHA=0）；也支持 Mode 11，本工程不用 |
| 时钟上限 | 10 MHz；本工程 DIV16 ≈ 4.5 MHz |
| id 寄存器 | `0xD0`：BMP280=`0x58`，BME280=`0x60` |
| 读写 | 读 `reg \| 0x80`，写 `reg & 0x7F` |
| 校准 | 上电读 `0x88` 起 24 字节，再补偿；原始 ADC 不能当温度/气压 |
| 配置 | `0xF5=0x24`（t_sb 62.5 ms，滤波 4），再写 `0xF4=0x57`（osrs_t×2、osrs_p×16、normal） |
| 测量 | 先 `ReadTemp`（0.01℃，写 `t_fine`）再 `ReadPressure`（Pa）。**无湿度**（BME280 才有，本驱动不读） |

## 接线

板级表见 [stm32f103-peripherals.md § BMP / BME280](../../hardware/stm32f103-peripherals.md#bmp--bme280)。

| 模块脚 | MCU |
|--------|-----|
| VCC | 面包板 3.3V（≤3.6V，禁止 5V） |
| GND | 面包板 GND |
| SCK | PA5 |
| SDI | PA7 |
| SDO | PA6（**禁止接地**） |
| CSB | PA3（空闲高） |

短线直连，SPI 不加 4.7 kΩ。

## 排错

| 现象 | 检查 |
|------|------|
| ID=`0x00` / `0xFF` | SDO 是否接地、CSB 是否空闲高，是否 Mode 0、3.3V、共地 |
| 无响应且像 I2C | CSB 悬空，芯片锁在 I2C，须断电再拉 CSB |
| 温度几千上万 | 未读校准，或先读气压再读温度 |
| 气压离谱 | 校准连读错误；或未先 `ReadTemp` |
| ID 对但数据不动 | `ctrl_meas` 须为 normal（`0x57`），不是休眠 |

OLED：每秒与时钟同拍，`SH1106_DrawTemp` 画在右下（y=48），时钟几何不变。串口 `temp xx.xx C  press … Pa`。

源码：[`bmp280.c`](../../../projects/f103-manual-reg/src/driver/bmp280.c)
