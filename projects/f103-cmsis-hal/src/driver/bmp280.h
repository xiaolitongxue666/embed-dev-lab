/**
 * @file    bmp280.h
 * @brief   BMP280 / BME280 SPI：ID、校准、补偿温度 / 气压
 *
 * 接线：SCK/SDI/SDO/CSB → PA5/PA7/PA6/PA3；SDO 禁止接地。
 * 访问前 SPI1_SetMode0，只拉 PA3。无湿度（BME280 才有，本驱动不读）。
 * 须先 ReadTemp（写 t_fine）再 ReadPressure。
 */

#ifndef BMP280_H
#define BMP280_H

#include <stdint.h>

/** id 寄存器期望值：BMP280=0x58，BME280=0x60 */
#define BMP280_ID_VALUE     0x58U
#define BME280_ID_VALUE     0x60U

uint8_t BMP280_ReadReg(uint8_t reg);
void BMP280_WriteReg(uint8_t reg, uint8_t value);
void BMP280_ReadMultiReg(uint8_t reg, uint8_t *buf, uint32_t len);
uint8_t BMP280_ReadID(void);

/**
 * @brief  读 24 字节校准，写 config/ctrl_meas 进 normal
 * @return 1=成功，0=ID 不符
 */
uint8_t BMP280_Init(void);

/** 补偿温度，单位 0.01℃（2510 = 25.10℃）；更新 t_fine */
int32_t BMP280_ReadTemp(void);

/** 补偿气压，单位 Pa；须先 ReadTemp */
uint32_t BMP280_ReadPressure(void);

#endif /* BMP280_H */
