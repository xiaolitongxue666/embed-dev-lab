/**
 * @file    main.c
 * @brief   手写寄存器 demo：LED、USART1、ADC DMA、TIM2 风扇、I2C OLED、SPI BMP280
 *
 * 初始化含 Key_ExtiInit、TIM2_PWM_Init。主循环不访问 LSM6。
 * BMP280 失败时 OLED 画 0.00C。
 * 周期任务走 SysTick 软件定时 flag，不忙等。
 * 串口走 LOG_*（时间 + 级别），不用裸 printf。
 *
 * @see     doc/projects/f103-manual-reg.md
 */

#include "adc.h"
#include "bmp280.h"
#include "gpio.h"
#include "gpioc_bitband.h"
#include "i2c.h"
#include "key.h"
#include "log.h"
#include "sh1106.h"
#include "spi.h"
#include "systick.h"
#include "tim2.h"
#include "timer_event.h"
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
    unsigned int knob_raw;
    unsigned int knob_mv;
    unsigned int fan_duty;
    unsigned char oled_ok;
    SysTime_t now;
    unsigned int clock_h;
    unsigned int clock_m;
    unsigned int clock_s;
    unsigned int clock_cs;
    int temp_centi_cached;
    unsigned int press_pa;
    unsigned int temp_abs;
    const char *led_str;
    const char *key_str;

    GPIOC_Init();
    GPIOB_Init();
    Key_ExtiInit();
    USART1_Init();
    USART1_SetRxHandle(USART1_EchoFrame);
    ADC1_Init();
    TIM2_PWM_Init();
    SysTick_Init();

    LOG_I("Stm32 manual reg BMP280 + JY003 fan + SH1106 I2C demo start");
    LOG_I("LSM6DS3 deferred (driver linked, main unused)");

    SPI1_Init();
    I2C1_Init();

    delay_boot_20ms();

    bmp_id = BMP280_ReadID();
    LOG_I("BMP280 ID=0x%02X (expect 0x%02X / BME 0x%02X)",
          (unsigned int)bmp_id,
          (unsigned int)BMP280_ID_VALUE,
          (unsigned int)BME280_ID_VALUE);
    if ((bmp_id != BMP280_ID_VALUE) && (bmp_id != BME280_ID_VALUE)) {
        LOG_E("BMP280 ID mismatch; check CSB/SDO/Mode0/3V3/GND");
    }

    bmp_ok = BMP280_Init();
    if (bmp_ok != 0U) {
        LOG_I("BMP280 init: calib + normal, osrs_t x2 / osrs_p x16");
    } else {
        LOG_E("BMP280 init failed");
    }

    oled_ok = 0U;
    temp_centi_cached = 0;
    if (I2C1_Probe(SH1106_ADDR_WR) != 0U) {
        LOG_I("SH1106 ACK addr=0x78");
        SH1106_Init();
        SysTick_SetMs(0U);
        oled_ok = 1U;
    } else {
        LOG_E("SH1106 NACK; check PB6/PB7/3V3/GND");
    }

    (void)TimerEvent_Register((unsigned char)TIMER_EVT_OLED_CLOCK, 50U,
                              (unsigned char)TIMER_MODE_PERIODIC);
    (void)TimerEvent_Register((unsigned char)TIMER_EVT_LED_LOG, 1000U,
                              (unsigned char)TIMER_MODE_PERIODIC);

    for (;;) {
        USART1_ProcessRx();

        if (Key_TakeEdge() != 0U) {
            if (PBin(KEY_PIN) != 0U) {
                LOG_I("PB13 KEY irq high");
            } else {
                LOG_I("PB13 KEY irq low");
            }
        }

        knob_raw = ADC1_ReadRaw();
        fan_duty = knob_to_fan_duty(knob_raw);
        Fan_SetDuty(fan_duty);

        if (TimerEvent_TakeFlag((unsigned char)TIMER_EVT_OLED_CLOCK) != 0U) {
            if (oled_ok != 0U) {
                SysTime_Get(&now);
                clock_h = (now.sec / 3600U) % 100U;
                clock_m = (now.sec / 60U) % 60U;
                clock_s = now.sec % 60U;
                clock_cs = now.ms / 10U;
                SH1106_Clear();
                SH1106_DrawClock(clock_h, clock_m, clock_s, clock_cs);
                SH1106_DrawTemp(temp_centi_cached);
                SH1106_Refresh();
            }
        }

        if (TimerEvent_TakeFlag((unsigned char)TIMER_EVT_LED_LOG) != 0U) {
            SysTime_Get(&now);
            clock_h = (now.sec / 3600U) % 100U;
            clock_m = (now.sec / 60U) % 60U;
            clock_s = now.sec % 60U;
            clock_cs = now.ms / 10U;

            PCout(BOARD_LED_PIN) ^= 1U;
            PBout(EXT_LED_PIN) ^= 1U;
            led_str = (PCout(BOARD_LED_PIN) != 0U) ? "on" : "off";
            key_str = (PBin(KEY_PIN) != 0U) ? "high" : "low";
            knob_mv = (knob_raw * ADC1_VDDA_MV) / ADC1_FULL_SCALE;
            if (bmp_ok != 0U) {
                temp_centi_cached = BMP280_ReadTemp();
                press_pa = BMP280_ReadPressure();
                temp_abs = (temp_centi_cached < 0)
                               ? (unsigned int)(-temp_centi_cached)
                               : (unsigned int)temp_centi_cached;
                LOG_D("LED %s  KEY %s  knob raw=%u mv=%u duty=%u  clock %02u:%02u:%02u:%02u  temp %s%u.%02u C press %u Pa",
                      led_str, key_str, knob_raw, knob_mv, fan_duty,
                      clock_h, clock_m, clock_s, clock_cs,
                      (temp_centi_cached < 0) ? "-" : "",
                      temp_abs / 100U, temp_abs % 100U, press_pa);
            } else {
                temp_centi_cached = 0;
                LOG_D("LED %s  KEY %s  knob raw=%u mv=%u duty=%u  clock %02u:%02u:%02u:%02u",
                      led_str, key_str, knob_raw, knob_mv, fan_duty,
                      clock_h, clock_m, clock_s, clock_cs);
            }
        }
    }
}
