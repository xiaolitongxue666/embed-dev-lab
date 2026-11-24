/**
 * @file    bmp280.h
 * @brief   BMP280 / BME280 SPI：ID、校准、补偿温度 / 气压
 *
 * 接线：SCK/SDI/SDO/CSB → PA5/PA7/PA6/PA3；SDO 禁止接地。
 * 访问前 SPI1_SetMode0，只拉 PA3。无湿度（BME280 才有，本驱动不读）。
 * 须先 ReadTemp（写 t_fine）再 ReadPressure。
 *
 * @see     doc/reference/bmp280/README.md
 * @see     doc/hardware/stm32f103-peripherals.md
 */

#ifndef BMP280_H
#define BMP280_H

/** id 寄存器期望值：BMP280=0x58，BME280=0x60 */
#define BMP280_ID_VALUE     0x58U
#define BME280_ID_VALUE     0x60U

unsigned char BMP280_ReadReg(unsigned char reg);
void BMP280_WriteReg(unsigned char reg, unsigned char value);
void BMP280_ReadMultiReg(unsigned char reg, unsigned char *buf, unsigned int len);
unsigned char BMP280_ReadID(void);

/**
 * @brief  读 24 字节校准，写 config/ctrl_meas 进 normal
 * @return 1=成功，0=ID 不符
 */
unsigned char BMP280_Init(void);

/** 补偿温度，单位 0.01℃（2510 = 25.10℃）；更新 t_fine */
int BMP280_ReadTemp(void);

/** 补偿气压，单位 Pa；须先 ReadTemp */
unsigned int BMP280_ReadPressure(void);

#endif /* BMP280_H */
