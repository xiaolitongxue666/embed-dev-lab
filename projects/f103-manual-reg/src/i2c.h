/**
 * @file    i2c.h
 * @brief   I2C1 硬件主机写（阻塞 + 超时）— 非 GPIO 软模拟
 *
 * 默认映射：PB6=SCL，PB7=SDA，复用开漏 50 MHz。勿 USART1_REMAP。
 * 从机地址参数为 **8 位写地址**（屏幕 `0x78`），写入 DR 时不再左移。
 *
 * @see     i2c.c
 * @see     doc/reference/stm32f103/md/topics/i2c1-master-polling.md
 * @see     doc/hardware/stm32f103-peripherals.md
 */

#ifndef I2C_H
#define I2C_H

void I2C1_Init(void);

/**
 * @brief  主机写：START + 8 位写地址 + buf[len] + STOP
 * @param  addr8  8 位写地址（屏幕传 0x78，禁止再 << 1）
 * @return 1 成功，0 超时或 AF（无 ACK）
 */
unsigned char I2C1_Write(unsigned char addr8, const unsigned char *buf, unsigned int len);

/**
 * @brief  只发 8 位写地址，看从机是否 ACK
 * @param  addr8  8 位写地址（屏幕传 0x78）
 * @return 1 ACK，0 NACK/超时
 */
unsigned char I2C1_Probe(unsigned char addr8);

#endif /* I2C_H */
