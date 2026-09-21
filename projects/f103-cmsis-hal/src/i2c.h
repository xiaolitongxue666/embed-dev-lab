/**
 * @file    i2c.h
 * @brief   I2C1 主机写：PB6/PB7，地址为 8 位写地址（SH1106 用 0x78）
 */

#ifndef I2C_H
#define I2C_H

#include "main.h"

extern I2C_HandleTypeDef hi2c1;

void MX_I2C1_Init(void);
uint8_t I2C1_Write(uint8_t addr8, const uint8_t *buf, uint32_t len);
uint8_t I2C1_WriteDma(uint8_t addr8, uint8_t ctrl, const uint8_t *mem, uint32_t len);
uint8_t I2C1_Probe(uint8_t addr8);

#endif /* I2C_H */
