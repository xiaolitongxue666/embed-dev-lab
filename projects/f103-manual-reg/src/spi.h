/**
 * @file    spi.h
 * @brief   SPI1 四线主机（Mode 3）+ PA4 软件片选
 *
 * 引脚：PA5=SCK→模块 SCL；PA7=MOSI→SDA；PA6=MISO←SAO；PA4=CS；供电 3V3/GND。
 *
 * @see     spi.c — 接线表与寄存器初始化
 * @see     doc/hardware/stm32f103-peripherals.md
 * @see     doc/reference/lsm6ds3/md/topics/electrical-spi-timing.md
 */

#ifndef SPI_H
#define SPI_H

void SPI1_Init(void);
unsigned char SPI1_TransferByte(unsigned char tx);
void SPI1_CsLow(void);
void SPI1_CsHigh(void);

#endif /* SPI_H */
