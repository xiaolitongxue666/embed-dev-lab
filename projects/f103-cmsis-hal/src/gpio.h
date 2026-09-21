/**
 * @file    gpio.h
 * @brief   板级 GPIO：PC13 LED、SPI 片选
 *
 * PC13 属 Backup 域，须先开 PWR 并解除 DBP。
 * PA3 = BMP280 CSB，PA8 = LSM6DS3 CS，空闲高、低有效。
 */

#ifndef GPIO_H
#define GPIO_H

#include "main.h"

#define BOARD_LED_PORT   GPIOC
#define BOARD_LED_PIN    GPIO_PIN_13
#define BMP280_CS_PORT   GPIOA
#define BMP280_CS_PIN    GPIO_PIN_3
#define LSM6DS3_CS_PORT  GPIOA
#define LSM6DS3_CS_PIN   GPIO_PIN_8
#define EXT_LED_PORT     GPIOB
#define EXT_LED_PIN      GPIO_PIN_12
#define KEY_PORT         GPIOB
#define KEY_PIN          GPIO_PIN_13

void MX_GPIO_Init(void);

#endif /* GPIO_H */
