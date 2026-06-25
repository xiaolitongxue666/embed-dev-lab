/**
 * @file    main.h
 * @brief   f103-cmsis-hal 应用公共头
 *
 * @note    包含 HAL 主头 stm32f1xx_hal.h（由 stm32f1xx_hal_conf.h 裁剪模块）
 *          Error_Handler 供 HAL 库在错误路径回调（本 demo 时钟失败时未调用）
 */

#ifndef MAIN_H
#define MAIN_H

#include "stm32f1xx_hal.h"

void Error_Handler(void);

#endif /* MAIN_H */
