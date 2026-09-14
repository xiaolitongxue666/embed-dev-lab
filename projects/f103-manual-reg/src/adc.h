/**
 * @file    adc.h
 * @brief   ADC1 通道 0（PA0）手写寄存器：校准、单次软件触发
 *
 * 不链接 CMSIS。PA0 = ADC12_IN0（非 FT，模块须 3.3 V）；B12/B13 侧排针无 ADC。
 * PCLK2=72 MHz 时 ADCPRE=/6 → ADC 时钟 12 MHz。
 * 12-bit 右对齐；电压按 VDDA=3.3 V 用整数换算：mv = raw * 3300 / 4095。
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
 * @brief  开启 ADC1 时钟、PA0 模拟输入、校准、规则组 CH0
 */
void ADC1_Init(void);

/**
 * @brief  软件触发一次规则转换，等待 EOC，返回 12-bit 右对齐值
 */
unsigned int ADC1_ReadRaw(void);

#endif /* ADC_H */
