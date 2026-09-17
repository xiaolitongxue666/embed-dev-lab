/**
 * @file    main.c
 * @brief   手写寄存器 demo：LED、USART1、ADC DMA、TIM2 风扇、I2C OLED、SPI BMP280
 *
 * 初始化含 Key_ExtiInit、TIM2_PWM_Init。主循环不访问 LSM6。
 * BMP280 失败时 OLED 画 0.00C。
 *
 * @see     doc/projects/f103-manual-reg.md
 */

#include <stdio.h>

#include "adc.h"
#include "bmp280.h"
#include "gpio.h"
#include "gpioc_bitband.h"
#include "i2c.h"
#include "key.h"
#include "sh1106.h"
#include "spi.h"
#include "systick.h"
#include "tim2.h"
#include "usart.h"

static void delay(volatile unsigned int count)
{
    while (count != 0U) {
        count--;
    }
}

static void delay_boot_20ms(void)
{
    delay(0xFFFFFU);
    delay(0xFFFFFU);
}

static void USART1_EchoFrame(const unsigned char *data, unsigned int len)
{
    USART1_Write((const char *)data, (int)len);
}

static unsigned int knob_to_fan_duty(unsigned int knob_raw)
{
    if (knob_raw <= FAN_KNOB_DEADZONE) {
        return 0U;
    }
    return (knob_raw * FAN_DUTY_MAX) / ADC1_FULL_SCALE;
}

int main(void)
{
    unsigned char bmp_id;
    unsigned char bmp_ok;
    unsigned int led_phase;
    unsigned int knob_raw;
    unsigned int knob_mv;
    unsigned int fan_duty;
    unsigned char oled_ok;
    unsigned int last_sec;
    unsigned int elapsed_sec;
    unsigned int clock_h;
    unsigned int clock_m;
    unsigned int clock_s;
    int temp_centi;
    unsigned int temp_abs;
    unsigned int press_pa;

    GPIOC_Init();
    GPIOB_Init();
    Key_ExtiInit();
    USART1_Init();
    USART1_SetRxHandle(USART1_EchoFrame);
    ADC1_Init();
    TIM2_PWM_Init();
    SysTick_Init();

    printf("Stm32 manual reg BMP280 + JY003 fan + SH1106 I2C demo start\n");
    printf("LSM6DS3 deferred (not connected)\n");

    SPI1_Init();
    I2C1_Init();

    delay_boot_20ms();

    bmp_id = BMP280_ReadID();
    printf("BMP280 ID=0x%02X (expect 0x%02X / BME 0x%02X)\n",
           (unsigned int)bmp_id,
           (unsigned int)BMP280_ID_VALUE,
           (unsigned int)BME280_ID_VALUE);
    if ((bmp_id != BMP280_ID_VALUE) && (bmp_id != BME280_ID_VALUE)) {
        printf("BMP280 ID mismatch; check CSB/SDO/Mode0/3V3/GND\n");
    }

    bmp_ok = BMP280_Init();
    if (bmp_ok != 0U) {
        printf("BMP280 init: calib + normal, osrs_t x2 / osrs_p x16\n");
    } else {
        printf("BMP280 init failed\n");
    }

    oled_ok = 0U;
    if (I2C1_Probe(SH1106_ADDR_WR) != 0U) {
        printf("SH1106 ACK addr=0x78\n");
        SH1106_Init();
        SysTick_SetMs(0U);
        oled_ok = 1U;
    } else {
        printf("SH1106 NACK; check PB6/PB7/3V3/GND\n");
    }

    led_phase = 0U;
    last_sec = 0xFFFFFFFFU;
    for (;;) {
        USART1_ProcessRx();

        if (Key_TakeEdge() != 0U) {
            if (PBin(KEY_PIN) != 0U) {
                printf("PB13 KEY irq high\n");
            } else {
                printf("PB13 KEY irq low\n");
            }
        }

        knob_raw = ADC1_ReadRaw();
        fan_duty = knob_to_fan_duty(knob_raw);
        Fan_SetDuty(fan_duty);

        elapsed_sec = SysTick_GetMs() / 1000U;
        if ((oled_ok != 0U) && (elapsed_sec != last_sec)) {
            last_sec = elapsed_sec;
            clock_h = (elapsed_sec / 3600U) % 24U;
            clock_m = (elapsed_sec / 60U) % 60U;
            clock_s = elapsed_sec % 60U;
            SH1106_Clear();
            SH1106_DrawClock(clock_h, clock_m, clock_s);
            if (bmp_ok != 0U) {
                temp_centi = BMP280_ReadTemp();
                press_pa = BMP280_ReadPressure();
                SH1106_DrawTemp(temp_centi);
                temp_abs = (temp_centi < 0) ? (unsigned int)(-temp_centi)
                                            : (unsigned int)temp_centi;
                printf("temp %s%u.%02u C  press %u Pa\n",
                       (temp_centi < 0) ? "-" : "",
                       temp_abs / 100U, temp_abs % 100U, press_pa);
            } else {
                SH1106_DrawTemp(0);
            }
            SH1106_Refresh();
            printf("clock %02u:%02u:%02u\n", clock_h, clock_m, clock_s);
        }

        led_phase++;
        if (led_phase >= 200U) {
            led_phase = 0U;
            PCout(BOARD_LED_PIN) ^= 1U;
            PBout(EXT_LED_PIN) ^= 1U;
            if (PCout(BOARD_LED_PIN) != 0U) {
                printf("PC13 LED on\n");
                printf("PB12 LED on\n");
            } else {
                printf("PC13 LED off\n");
                printf("PB12 LED off\n");
            }
            if (PBin(KEY_PIN) != 0U) {
                printf("PB13 KEY high\n");
            } else {
                printf("PB13 KEY low\n");
            }
            knob_mv = (knob_raw * ADC1_VDDA_MV) / ADC1_FULL_SCALE;
            printf("knob raw=%u mv=%u duty=%u\n", knob_raw, knob_mv, fan_duty);
            if (bmp_ok != 0U) {
                temp_centi = BMP280_ReadTemp();
                press_pa = BMP280_ReadPressure();
                temp_abs = (temp_centi < 0) ? (unsigned int)(-temp_centi)
                                            : (unsigned int)temp_centi;
                printf("temp %s%u.%02u C  press %u Pa\n",
                       (temp_centi < 0) ? "-" : "",
                       temp_abs / 100U, temp_abs % 100U, press_pa);
            }
        }

        delay(0x7FFFU);
    }
}
