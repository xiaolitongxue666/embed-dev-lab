/**
 * @file    adc.h
 * @brief   ADC1 通道 0（PA0）：校准后 DMA1 CH1 循环写入 raw
 *
 * 不链接 CMSIS。PA0 = ADC12_IN0（非 FT，模块须 3.3 V）。
 * PCLK2=72 MHz 时 ADCPRE=/6 → ADC 时钟 12 MHz。
 * 连续转换 + CR2.DMA；CH1 循环、16-bit、无 MINC（RM0008 ADC/DMA）。
 * ADC1_ReadRaw 只读最近一次 DMA 结果，不再 SWSTART 空等。
 *
 * @see     adc.c
 * @see     doc/hardware/stm32f103c8t6-pinout.md
 */

#ifndef ADC_H
#define ADC_H

/** 假定 VDDA = 3.3 V，用于 raw→毫伏 */
#define ADC1_VDDA_MV 3300U
/** 12-bit 满量程 */
#define ADC1_FULL_SCALE 4095U

/**
 * @brief  开启 ADC1、PA0 模拟、校准、DMA1 CH1 循环、连续转换
 */
void ADC1_Init(void);

/**
 * @brief  返回 DMA 最近写入的 12-bit 右对齐值
 */
unsigned int ADC1_ReadRaw(void);

#endif /* ADC_H */
