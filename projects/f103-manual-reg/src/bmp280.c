/**
 * @file    bmp280.c
 * @brief   BMP280 / BME280 SPI 寄存器访问（BST-BMP280-DS001）
 *
 * 读：地址最高位置 1（reg | 0x80）；写：最高位清 0（reg & 0x7F）。
 * 访问前切 Mode 0，只拉 PA3；结束后 CS 拉高。
 */

#include "bmp280.h"
#include "spi.h"

#define BMP280_REG_ID  0xD0U

unsigned char BMP280_ReadReg(unsigned char reg)
{
    unsigned char value;

    SPI1_SetMode0();
    BMP280_CsLow();
    (void)SPI1_TransferByte((unsigned char)(0x80U | (reg & 0x7FU)));
    value = SPI1_TransferByte(0x00U);
    BMP280_CsHigh();
    SPI1_SetMode3();

    return value;
}

void BMP280_WriteReg(unsigned char reg, unsigned char value)
{
    SPI1_SetMode0();
    BMP280_CsLow();
    (void)SPI1_TransferByte((unsigned char)(reg & 0x7FU));
    (void)SPI1_TransferByte(value);
    BMP280_CsHigh();
    SPI1_SetMode3();
}

unsigned char BMP280_ReadID(void)
{
    return BMP280_ReadReg(BMP280_REG_ID);
}
