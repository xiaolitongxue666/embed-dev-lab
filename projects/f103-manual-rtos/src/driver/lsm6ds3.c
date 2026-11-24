/**
 * @file    lsm6ds3.c
 * @brief   LSM6DS3 SPI 寄存器访问（驱动保留，main 不调用）
 *
 * 帧格式（DocID026899 §6.2）：首字节 RW|AD(6:0)，随后数据；MSB first。
 * 读：首字节 bit7=1；写：bit7=0。多字节依赖 CTRL3_C.IF_INC 复位默认 1。
 * 片选 PA8（LSM6DS3_Cs*）；与 BMP280 的 PA3 分开。本阶段不强制验 IMU。
 *
 * @see     doc/reference/lsm6ds3/md/topics/spi-protocol.md
 * @see     doc/reference/lsm6ds3/md/topics/registers-whoami-imu.md
 * @see     doc/reference/lsm6ds3/md/topics/electrical-spi-timing.md
 */

#include "lsm6ds3.h"
#include "spi.h"

#define LSM6DS3_REG_WHO_AM_I   0x0FU
#define LSM6DS3_REG_CTRL1_XL   0x10U
#define LSM6DS3_REG_CTRL2_G    0x11U
#define LSM6DS3_REG_STATUS     0x1EU
#define LSM6DS3_REG_OUTX_L_G   0x22U

/** CTRL1_XL：ODR=104 Hz，FS=±2 g → 0x40 */
#define LSM6DS3_CTRL1_XL_104HZ_2G  0x40U
/** CTRL2_G：ODR=104 Hz，FS=250 dps → 0x40 */
#define LSM6DS3_CTRL2_G_104HZ_250DPS  0x40U

#define LSM6DS3_STATUS_XLDA  (1U << 0)
#define LSM6DS3_STATUS_GDA   (1U << 1)

/**
 * @brief  读单寄存器
 */
unsigned char LSM6DS3_ReadReg(unsigned char reg)
{
    unsigned char value;

    LSM6DS3_CsLow();
    (void)SPI1_TransferByte((unsigned char)(0x80U | (reg & 0x7FU)));
    value = SPI1_TransferByte(0x00U);
    LSM6DS3_CsHigh();

    return value;
}

/**
 * @brief  写单寄存器
 */
void LSM6DS3_WriteReg(unsigned char reg, unsigned char value)
{
    LSM6DS3_CsLow();
    (void)SPI1_TransferByte((unsigned char)(reg & 0x7FU));
    (void)SPI1_TransferByte(value);
    LSM6DS3_CsHigh();
}

unsigned char LSM6DS3_ReadWhoAmI(void)
{
    return LSM6DS3_ReadReg(LSM6DS3_REG_WHO_AM_I);
}

/**
 * @brief  配置加速度计 / 陀螺仪 ODR 与满量程
 *
 * 调用前须已 SPI1_Init，且上电后延时 ≥ 20 ms（见 electrical-spi-timing.md）。
 */
void LSM6DS3_Init(void)
{
    LSM6DS3_WriteReg(LSM6DS3_REG_CTRL1_XL, LSM6DS3_CTRL1_XL_104HZ_2G);
    LSM6DS3_WriteReg(LSM6DS3_REG_CTRL2_G, LSM6DS3_CTRL2_G_104HZ_250DPS);
}

/**
 * @brief  从 OUTX_L_G 连读 12 字节拼装六轴
 *
 * 灵敏度（注释备查，本函数只返回 raw）：
 *   ±2 g → 0.061 mg/LSB；250 dps → 8.75 mdps/LSB（DocID026899 Table 3）。
 */
unsigned char LSM6DS3_ReadRaw(LSM6DS3_RawSample *out)
{
    unsigned char status;
    unsigned char buf[12];
    unsigned int i;

    if (out == 0) {
        return 0U;
    }

    status = LSM6DS3_ReadReg(LSM6DS3_REG_STATUS);
    if ((status & (LSM6DS3_STATUS_XLDA | LSM6DS3_STATUS_GDA)) == 0U) {
        return 0U;
    }

    LSM6DS3_CsLow();
    {
        unsigned char tx[13];
        unsigned char rx[13];

        tx[0] = (unsigned char)(0x80U | LSM6DS3_REG_OUTX_L_G);
        for (i = 1U; i < 13U; i++) {
            tx[i] = 0x00U;
        }
        if (SPI1_TransferBytes(tx, rx, 13U) == 0U) {
            LSM6DS3_CsHigh();
            return 0U;
        }
        for (i = 0U; i < 12U; i++) {
            buf[i] = rx[i + 1U];
        }
    }
    LSM6DS3_CsHigh();

    out->gx = (short)((unsigned short)buf[0] | ((unsigned short)buf[1] << 8));
    out->gy = (short)((unsigned short)buf[2] | ((unsigned short)buf[3] << 8));
    out->gz = (short)((unsigned short)buf[4] | ((unsigned short)buf[5] << 8));
    out->ax = (short)((unsigned short)buf[6] | ((unsigned short)buf[7] << 8));
    out->ay = (short)((unsigned short)buf[8] | ((unsigned short)buf[9] << 8));
    out->az = (short)((unsigned short)buf[10] | ((unsigned short)buf[11] << 8));

    return 1U;
}
