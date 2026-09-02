/**
 * @file    spi.h
 * @brief   SPI1 四线主机（Mode 3）+ PA4 软件片选 — 硬件 SPI，非 GPIO 软模拟
 *
 * 物理仍是标准 4 线：PA5=SCK，PA7=MOSI，PA6=MISO，PA4=CS（GPIO）。
 * 与「GPIO 一位一位翻线」的软 SPI 不同：本驱动用片内 SPI1，软件写 CR1/SR/DR，
 * 硬件完成移位；CS 仍用 GPIO（与软 SPI「先拉片选」相同）。
 * 模块丝印：SCL←SCK，SDA←MOSI，SAO→MISO，CS←PA4；供电 3V3/GND。
 * PA4–PA7 手册未标 FT；默认映射；勿与 USART2 默认脚同时启用。
 *
 * @see     spi.c — 软 SPI vs 硬件 SPI、信号线 ↔ 寄存器、接线表
 * @see     doc/projects/f103-manual-reg.md
 * @see     doc/hardware/stm32f103-peripherals.md
 * @see     doc/hardware/stm32f103c8t6-pinout.md
 * @see     doc/reference/lsm6ds3/md/topics/electrical-spi-timing.md
 */

#ifndef SPI_H
#define SPI_H

void SPI1_Init(void);
unsigned char SPI1_TransferByte(unsigned char tx);
void SPI1_CsLow(void);
void SPI1_CsHigh(void);

#endif /* SPI_H */
