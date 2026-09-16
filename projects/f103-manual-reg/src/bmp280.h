/**
 * @file    bmp280.h
 * @brief   BMP280 / BME280 SPI 驱动（读芯片 ID）
 *
 * 接线：SCK/SDI/SDO/CSB → PA5/PA7/PA6/PA3；SDO 禁止接地。
 * 访问前 SPI1_SetMode0，只拉 PA3。
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
unsigned char BMP280_ReadID(void);

#endif /* BMP280_H */
