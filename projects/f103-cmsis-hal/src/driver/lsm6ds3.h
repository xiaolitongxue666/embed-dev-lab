/**
 * @file    lsm6ds3.h
 * @brief   LSM6DS3 / LSM6DS3TR SPI 驱动（WHO_AM_I + 六轴 raw）
 *
 * CS=PA8（与 BMP280 的 PA3 分开）。SCL/SDA/SAO ↔ PA5/7/6。
 * 本工程 main 不调用本驱动，仅链入 ELF。
 */

#ifndef LSM6DS3_H
#define LSM6DS3_H

#include <stdint.h>

/** WHO_AM_I 期望值（LSM6DS3 / LSM6DS3TR） */
#define LSM6DS3_WHO_AM_I_VALUE  0x69U

/** 六轴 raw 样本（二进制补码 LSB） */
typedef struct {
    int16_t gx;
    int16_t gy;
    int16_t gz;
    int16_t ax;
    int16_t ay;
    int16_t az;
} LSM6DS3_RawSample;

void LSM6DS3_Init(void);
uint8_t LSM6DS3_ReadWhoAmI(void);
uint8_t LSM6DS3_ReadReg(uint8_t reg);
void LSM6DS3_WriteReg(uint8_t reg, uint8_t value);
uint8_t LSM6DS3_ReadRaw(LSM6DS3_RawSample *out);

#endif /* LSM6DS3_H */
