/**
 * @file    lsm6ds3.h
 * @brief   LSM6DS3 / LSM6DS3TR SPI 驱动（WHO_AM_I + 六轴 raw）
 *
 * 接线见 spi.c / doc/hardware/stm32f103-peripherals.md（SCL/SDA/SAO/CS ↔ PA5/7/6/4）。
 *
 * @see     doc/reference/lsm6ds3/md/topics/spi-protocol.md
 * @see     doc/reference/lsm6ds3/md/topics/registers-whoami-imu.md
 */

#ifndef LSM6DS3_H
#define LSM6DS3_H

/** WHO_AM_I 期望值（LSM6DS3 / LSM6DS3TR） */
#define LSM6DS3_WHO_AM_I_VALUE  0x69U

/** 六轴 raw 样本（二进制补码 LSB） */
typedef struct {
    short gx;
    short gy;
    short gz;
    short ax;
    short ay;
    short az;
} LSM6DS3_RawSample;

void LSM6DS3_Init(void);
unsigned char LSM6DS3_ReadWhoAmI(void);
unsigned char LSM6DS3_ReadReg(unsigned char reg);
void LSM6DS3_WriteReg(unsigned char reg, unsigned char value);
/**
 * @brief  若 STATUS 指示有新数据则读 12 字节输出
 * @param  out  非 NULL 时写入样本
 * @return 1=已更新 out；0=尚无新数据
 */
unsigned char LSM6DS3_ReadRaw(LSM6DS3_RawSample *out);

#endif /* LSM6DS3_H */
