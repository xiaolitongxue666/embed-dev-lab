/**
 * @file    key.h
 * @brief   PB13 按键：EXTI13 双沿
 */

#ifndef KEY_H
#define KEY_H

#include "main.h"

void Key_ExtiInit(void);
uint8_t Key_TakeEdge(void);

#endif /* KEY_H */
