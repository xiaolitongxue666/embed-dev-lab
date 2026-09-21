/**
 * @file    lsm6ds3.c
 * @brief   LSM6DS3 SPI 寄存器访问（驱动保留，main 不调用）
 *
 * 读：首字节 bit7=1；写：bit7=0。片选 PA8。默认 SPI Mode 3。
 */

#include "lsm6ds3.h"
#include "spi.h"

#define LSM6DS3_REG_WHO_AM_I   0x0FU
#define LSM6DS3_REG_CTRL1_XL   0x10U
#define LSM6DS3_REG_CTRL2_G    0x11U
#define LSM6DS3_REG_STATUS     0x1EU
#define LSM6DS3_REG_OUTX_L_G   0x22U

#define LSM6DS3_CTRL1_XL_104HZ_2G     0x40U
#define LSM6DS3_CTRL2_G_104HZ_250DPS  0x40U

#define LSM6DS3_STATUS_XLDA  (1U << 0)
#define LSM6DS3_STATUS_GDA   (1U << 1)

uint8_t LSM6DS3_ReadReg(uint8_t reg)
{
    uint8_t value;

    SPI1_SetMode3();
    LSM6DS3_CsLow();
    (void)SPI1_TransferByte((uint8_t)(0x80U | (reg & 0x7FU)));
    value = SPI1_TransferByte(0x00U);
    LSM6DS3_CsHigh();

    return value;
}

void LSM6DS3_WriteReg(uint8_t reg, uint8_t value)
{
    SPI1_SetMode3();
    LSM6DS3_CsLow();
    (void)SPI1_TransferByte((uint8_t)(reg & 0x7FU));
    (void)SPI1_TransferByte(value);
    LSM6DS3_CsHigh();
}

uint8_t LSM6DS3_ReadWhoAmI(void)
{
    return LSM6DS3_ReadReg(LSM6DS3_REG_WHO_AM_I);
}

void LSM6DS3_Init(void)
{
    LSM6DS3_WriteReg(LSM6DS3_REG_CTRL1_XL, LSM6DS3_CTRL1_XL_104HZ_2G);
    LSM6DS3_WriteReg(LSM6DS3_REG_CTRL2_G, LSM6DS3_CTRL2_G_104HZ_250DPS);
}

uint8_t LSM6DS3_ReadRaw(LSM6DS3_RawSample *out)
{
    uint8_t status;
    uint8_t buf[12];
    uint32_t i;

    if (out == 0) {
        return 0U;
    }

    status = LSM6DS3_ReadReg(LSM6DS3_REG_STATUS);
    if ((status & (LSM6DS3_STATUS_XLDA | LSM6DS3_STATUS_GDA)) == 0U) {
        return 0U;
    }

    SPI1_SetMode3();
    LSM6DS3_CsLow();
    (void)SPI1_TransferByte((uint8_t)(0x80U | LSM6DS3_REG_OUTX_L_G));
    for (i = 0U; i < 12U; i++) {
        buf[i] = SPI1_TransferByte(0x00U);
    }
    LSM6DS3_CsHigh();

    out->gx = (int16_t)((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));
    out->gy = (int16_t)((uint16_t)buf[2] | ((uint16_t)buf[3] << 8));
    out->gz = (int16_t)((uint16_t)buf[4] | ((uint16_t)buf[5] << 8));
    out->ax = (int16_t)((uint16_t)buf[6] | ((uint16_t)buf[7] << 8));
    out->ay = (int16_t)((uint16_t)buf[8] | ((uint16_t)buf[9] << 8));
    out->az = (int16_t)((uint16_t)buf[10] | ((uint16_t)buf[11] << 8));

    return 1U;
}
