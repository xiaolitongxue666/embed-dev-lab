/**
 * @file    gpio.h
 * @brief   板级 GPIO：PC13 / PB12 LED、PB13 上拉输入
 *
 * @see     gpio.c
 * @see     doc/learn/gpio-eight-modes.md
 */

#ifndef GPIO_H
#define GPIO_H

#define BOARD_LED_PIN 13U
#define EXT_LED_PIN   12U
#define KEY_PIN       13U

void GPIOC_Init(void);
void GPIOB_Init(void);

#endif /* GPIO_H */
