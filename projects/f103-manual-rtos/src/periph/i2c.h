/**
 * @file    i2c.h
 * @brief   I2C1 硬件主机写 — 非 GPIO 软模拟
 *
 * 默认映射：PB6=SCL，PB7=SDA，复用开漏 50 MHz。勿 USART1_REMAP。
 * 从机地址参数为 **8 位写地址**（屏幕 `0x78`），写入 DR 时不再左移。
 *
 * 短包（命令 / Probe）轮询。页数据：软件发 START+地址+0x40，DMA1 CH6 搬后续字节，
 * TC 中断里等 BTF 再 STOP。DMA 不产生起停、地址、控制字节。
 *
 * @see     i2c.c
 * @see     doc/reference/stm32f103/md/topics/i2c1-master-polling.md
 * @see     doc/hardware/stm32f103-peripherals.md
 */

#ifndef I2C_H
#define I2C_H

void I2C1_Init(void);

/**
 * @brief  主机写：START + 8 位写地址 + buf[len] + STOP（轮询）
 * @param  addr8  8 位写地址（屏幕传 0x78，禁止再 << 1）
 * @return 1 成功，0 超时或 AF（无 ACK）
 */
unsigned char I2C1_Write(unsigned char addr8, const unsigned char *buf, unsigned int len);

/**
 * @brief  主机写大数据：START + addr8 + ctrl 轮询，其后 len 字节 DMA1 CH6
 * @param  addr8  8 位写地址（屏幕传 0x78）
 * @param  ctrl   控制字节（页数据为 0x40）
 * @param  mem    源缓冲，须为全局/静态，禁止局部栈
 * @return 1 成功，0 超时或 AF
 */
unsigned char I2C1_WriteDma(unsigned char addr8, unsigned char ctrl,
                            const unsigned char *mem, unsigned int len);

/**
 * @brief  只发 8 位写地址，看从机是否 ACK
 * @param  addr8  8 位写地址（屏幕传 0x78）
 * @return 1 ACK，0 NACK/超时
 */
unsigned char I2C1_Probe(unsigned char addr8);

/** 覆盖 startup 弱符号；非 HAL Callback。不开 I2C EV/ER */
void DMA1_Channel6_IRQHandler(void);

#endif /* I2C_H */
