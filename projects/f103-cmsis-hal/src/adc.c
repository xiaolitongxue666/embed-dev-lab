/**
 * @file    adc.c
 * @brief   ADC1 IN0（PA0）连续转换 + DMA1 CH1 循环
 */

#include "adc.h"

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
static uint16_t s_adc1_raw;

void MX_ADC1_Init(void)
{
    ADC_ChannelConfTypeDef ch = {0};

    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = ENABLE;
    hadc1.Init.NbrOfConversion = 1;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.NbrOfDiscConversion = 1;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    if (HAL_ADC_Init(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    ch.Channel = ADC_CHANNEL_0;
    ch.Rank = ADC_REGULAR_RANK_1;
    ch.SamplingTime = ADC_SAMPLETIME_55CYCLES_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &ch) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&s_adc1_raw, 1U) != HAL_OK) {
        Error_Handler();
    }
}

uint32_t ADC1_ReadRaw(void)
{
    return (uint32_t)s_adc1_raw;
}
