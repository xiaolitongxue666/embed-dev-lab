/**
 * @file    bmp280.c
 * @brief   BMP280 / BME280 SPI 寄存器与 Bosch 补偿（BST-BMP280-DS001）
 *
 * 读：地址最高位置 1（reg | 0x80）；写：最高位清 0（reg & 0x7F）。
 * 访问前切 Mode 0，只拉 PA3；结束后 CS 拉高。无湿度寄存器。
 */

#include "bmp280.h"
#include "spi.h"
#include "systick.h"

#define BMP280_REG_ID        0xD0U
#define BMP280_REG_CALIB     0x88U
#define BMP280_REG_PRESS     0xF7U
#define BMP280_REG_TEMP      0xFAU
#define BMP280_REG_CTRL_MEAS 0xF4U
#define BMP280_REG_CONFIG    0xF5U

/** t_sb=62.5 ms，滤波器系数 4 */
#define BMP280_CONFIG_VAL    0x24U
/** osrs_t×2、osrs_p×16、normal */
#define BMP280_CTRL_MEAS_VAL 0x57U

#define BMP280_CALIB_LEN     24U
#define BMP280_INIT_WAIT_MS  50U

struct bmp280_calib {
    unsigned int dig_T1;
    int dig_T2;
    int dig_T3;
    unsigned int dig_P1;
    int dig_P2;
    int dig_P3;
    int dig_P4;
    int dig_P5;
    int dig_P6;
    int dig_P7;
    int dig_P8;
    int dig_P9;
    int t_fine;
};

static struct bmp280_calib s_calib;

static unsigned int bmp280_u16_le(const unsigned char *p)
{
    return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static int bmp280_s16_le(const unsigned char *p)
{
    return (int)(short)((unsigned int)p[0] | ((unsigned int)p[1] << 8));
}

static void bmp280_delay_ms(unsigned int ms)
{
    unsigned int start;

    start = SysTick_GetMs();
    while ((SysTick_GetMs() - start) < ms) {
    }
}

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

void BMP280_ReadMultiReg(unsigned char reg, unsigned char *buf, unsigned int len)
{
    unsigned int i;

    if (buf == 0) {
        return;
    }

    SPI1_SetMode0();
    BMP280_CsLow();
    (void)SPI1_TransferByte((unsigned char)(0x80U | (reg & 0x7FU)));
    for (i = 0U; i < len; i++) {
        buf[i] = SPI1_TransferByte(0x00U);
    }
    BMP280_CsHigh();
    SPI1_SetMode3();
}

unsigned char BMP280_ReadID(void)
{
    return BMP280_ReadReg(BMP280_REG_ID);
}

unsigned char BMP280_Init(void)
{
    unsigned char buf[BMP280_CALIB_LEN];
    unsigned char id;

    id = BMP280_ReadID();
    if ((id != BMP280_ID_VALUE) && (id != BME280_ID_VALUE)) {
        return 0U;
    }

    BMP280_ReadMultiReg(BMP280_REG_CALIB, buf, BMP280_CALIB_LEN);

    s_calib.dig_T1 = bmp280_u16_le(&buf[0]);
    s_calib.dig_T2 = bmp280_s16_le(&buf[2]);
    s_calib.dig_T3 = bmp280_s16_le(&buf[4]);
    s_calib.dig_P1 = bmp280_u16_le(&buf[6]);
    s_calib.dig_P2 = bmp280_s16_le(&buf[8]);
    s_calib.dig_P3 = bmp280_s16_le(&buf[10]);
    s_calib.dig_P4 = bmp280_s16_le(&buf[12]);
    s_calib.dig_P5 = bmp280_s16_le(&buf[14]);
    s_calib.dig_P6 = bmp280_s16_le(&buf[16]);
    s_calib.dig_P7 = bmp280_s16_le(&buf[18]);
    s_calib.dig_P8 = bmp280_s16_le(&buf[20]);
    s_calib.dig_P9 = bmp280_s16_le(&buf[22]);
    s_calib.t_fine = 0;

    BMP280_WriteReg(BMP280_REG_CONFIG, BMP280_CONFIG_VAL);
    BMP280_WriteReg(BMP280_REG_CTRL_MEAS, BMP280_CTRL_MEAS_VAL);
    bmp280_delay_ms(BMP280_INIT_WAIT_MS);

    return 1U;
}

int BMP280_ReadTemp(void)
{
    unsigned char buf[3];
    int adc_T;
    int var1;
    int var2;

    BMP280_ReadMultiReg(BMP280_REG_TEMP, buf, 3U);
    adc_T = (int)(((unsigned int)buf[0] << 12) |
                  ((unsigned int)buf[1] << 4) |
                  ((unsigned int)buf[2] >> 4));

    var1 = ((((adc_T >> 3) - ((int)s_calib.dig_T1 << 1))) * s_calib.dig_T2) >> 11;
    var2 = (((((adc_T >> 4) - (int)s_calib.dig_T1) *
              ((adc_T >> 4) - (int)s_calib.dig_T1)) >> 12) *
            s_calib.dig_T3) >> 14;
    s_calib.t_fine = var1 + var2;

    return (s_calib.t_fine * 5 + 128) >> 8;
}

unsigned int BMP280_ReadPressure(void)
{
    unsigned char buf[3];
    int adc_P;
    long long var1;
    long long var2;
    long long p;

    BMP280_ReadMultiReg(BMP280_REG_PRESS, buf, 3U);
    adc_P = (int)(((unsigned int)buf[0] << 12) |
                  ((unsigned int)buf[1] << 4) |
                  ((unsigned int)buf[2] >> 4));

    var1 = (long long)s_calib.t_fine - 128000LL;
    var2 = var1 * var1 * (long long)s_calib.dig_P6;
    var2 = var2 + ((var1 * (long long)s_calib.dig_P5) << 17);
    var2 = var2 + (((long long)s_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (long long)s_calib.dig_P3) >> 8) +
           ((var1 * (long long)s_calib.dig_P2) << 12);
    var1 = (((((long long)1) << 47) + var1) * (long long)s_calib.dig_P1) >> 33;

    if (var1 == 0LL) {
        return 0U;
    }

    p = 1048576LL - (long long)adc_P;
    p = (((p << 31) - var2) * 3125LL) / var1;
    var1 = (((long long)s_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((long long)s_calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((long long)s_calib.dig_P7) << 4);

    /* Bosch Q24.8 → Pa */
    return (unsigned int)(p / 256LL);
}
