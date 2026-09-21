/**
 * @file    spi.h
 * @brief   SPI1 主机：PA5/PA6/PA7，双软件片选，运行时 Mode0/Mode3
 *
 * Init 默认 Mode 3（LSM6）。访问 BMP280 前 SPI1_SetMode0，只拉 PA3。
 * 传输为 HAL 轮询全双工；禁止同时拉两个 CS。
 */

#ifndef SPI_H
#define SPI_H

#include "main.h"

extern SPI_HandleTypeDef hspi1;

void MX_SPI1_Init(void);
uint8_t SPI1_TransferByte(uint8_t tx);
void SPI1_SetMode0(void);
void SPI1_SetMode3(void);
void BMP280_CsLow(void);
void BMP280_CsHigh(void);
void LSM6DS3_CsLow(void);
void LSM6DS3_CsHigh(void);

#endif /* SPI_H */
