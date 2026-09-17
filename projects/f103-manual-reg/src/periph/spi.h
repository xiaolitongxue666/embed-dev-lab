/**
 * @file    spi.h
 * @brief   SPI1 四线主机：双软件片选 + 运行时 Mode0/Mode3
 *
 * 物理仍是标准 4 线：PA5=SCK，PA7=MOSI，PA6=MISO；片选为 GPIO。
 * PA3 = BMP280 CSB（访问前 SPI1_SetMode0）；PA8 = LSM6 CS（访问前 SPI1_SetMode3）。
 * SPI1_Init 默认 Mode 3，两 CS 空闲高。
 * 与「GPIO 一位一位翻线」的软 SPI 不同：本驱动用片内 SPI1，软件写 CR1/SR/DR。
 * PA3、PA5–PA7 手册未标 FT；PA8 为 FT；默认映射；勿与 USART2 默认脚同时启用。
 *
 * @see     spi.c
 * @see     doc/hardware/stm32f103-peripherals.md
 * @see     doc/reference/bmp280/README.md
 */

#ifndef SPI_H
#define SPI_H

void SPI1_Init(void);
unsigned char SPI1_TransferByte(unsigned char tx);
/** 全双工 DMA1 CH3 TX + CH2 RX；len≤32；失败返回 0 */
unsigned char SPI1_TransferBytes(const unsigned char *tx, unsigned char *rx,
                                 unsigned int len);
void SPI1_SetMode0(void);
void SPI1_SetMode3(void);
void BMP280_CsLow(void);
void BMP280_CsHigh(void);
void LSM6DS3_CsLow(void);
void LSM6DS3_CsHigh(void);

#endif /* SPI_H */
