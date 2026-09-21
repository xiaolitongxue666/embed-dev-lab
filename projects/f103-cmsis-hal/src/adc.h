/**
 * @file    adc.h
 * @brief   ADC1 通道 0（PA0）：校准后 DMA1 CH1 循环写入 raw
 */

#ifndef ADC_H
#define ADC_H

#include "main.h"

#define ADC1_VDDA_MV    3300U
#define ADC1_FULL_SCALE 4095U

extern DMA_HandleTypeDef hdma_adc1;

void MX_ADC1_Init(void);
uint32_t ADC1_ReadRaw(void);

#endif /* ADC_H */
