/**
 * @file    main.c
 * @brief   CMSIS + HAL demo：LED、USART1、ADC、风扇、I2C OLED、SPI BMP280
 *
 * 主循环不访问 LSM6。BMP280 失败时 OLED 画 0.00C。
 * 周期任务走软件定时 flag。串口走 LOG_*（一次 HAL_UART_Transmit）。
 */

#include "adc.h"
#include "bmp280.h"
#include "gpio.h"
#include "i2c.h"
#include "key.h"
#include "log.h"
#include "lsm6ds3.h"
#include "main.h"
#include "sh1106.h"
#include "spi.h"
#include "tim.h"
#include "timer_event.h"
#include "usart.h"

/* 保留 LSM6 驱动进 ELF（--gc-sections）；main 运行时不调用 */
static void (*const s_lsm6ds3_keep)(void) = LSM6DS3_Init;

static void SystemClock_Config(void);
static uint32_t knob_to_fan_duty(uint32_t knob_raw);

int main(void)
{
    uint8_t bmp_id;
    uint8_t bmp_ok;
    uint8_t oled_ok;
    uint32_t knob_raw;
    uint32_t knob_mv;
    uint32_t fan_duty;
    SysTime_t now;
    uint32_t clock_h;
    uint32_t clock_m;
    uint32_t clock_s;
    uint32_t clock_cs;
    int32_t temp_centi_cached;
    uint32_t press_pa;
    uint32_t temp_abs;
    const char *led_str;
    const char *key_str;
    GPIO_PinState led_level;

    HAL_Init();
    (void)s_lsm6ds3_keep;
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    LOG_I("Stm32 cmsis hal BMP280 + JY003 fan + SH1106 I2C demo start");
    LOG_I("LSM6DS3 deferred (driver linked, main unused)");

    Key_ExtiInit();
    MX_ADC1_Init();
    MX_TIM2_Init();
    MX_SPI1_Init();
    MX_I2C1_Init();
    USART1_StartRxEcho();

    HAL_Delay(20U);

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
        SysTime_Reset();
        oled_ok = 1U;
    } else {
        LOG_E("SH1106 NACK; check PB6/PB7/3V3/GND");
    }

    (void)TimerEvent_Register((uint8_t)TIMER_EVT_OLED_CLOCK, 50U,
                              (uint8_t)TIMER_MODE_PERIODIC);
    (void)TimerEvent_Register((uint8_t)TIMER_EVT_LED_LOG, 1000U,
                              (uint8_t)TIMER_MODE_PERIODIC);

    for (;;) {
        if (Key_TakeEdge() != 0U) {
            if (HAL_GPIO_ReadPin(KEY_PORT, KEY_PIN) != GPIO_PIN_RESET) {
                LOG_I("PB13 KEY irq high");
            } else {
                LOG_I("PB13 KEY irq low");
            }
        }

        knob_raw = ADC1_ReadRaw();
        fan_duty = knob_to_fan_duty(knob_raw);
        Fan_SetDuty(fan_duty);

        if (TimerEvent_TakeFlag((uint8_t)TIMER_EVT_OLED_CLOCK) != 0U) {
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

        if (TimerEvent_TakeFlag((uint8_t)TIMER_EVT_LED_LOG) != 0U) {
            SysTime_Get(&now);
            clock_h = (now.sec / 3600U) % 100U;
            clock_m = (now.sec / 60U) % 60U;
            clock_s = now.sec % 60U;
            clock_cs = now.ms / 10U;

            led_level = HAL_GPIO_ReadPin(BOARD_LED_PORT, BOARD_LED_PIN);
            if (led_level != GPIO_PIN_RESET) {
                HAL_GPIO_WritePin(BOARD_LED_PORT, BOARD_LED_PIN, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(EXT_LED_PORT, EXT_LED_PIN, GPIO_PIN_RESET);
            } else {
                HAL_GPIO_WritePin(BOARD_LED_PORT, BOARD_LED_PIN, GPIO_PIN_SET);
                HAL_GPIO_WritePin(EXT_LED_PORT, EXT_LED_PIN, GPIO_PIN_SET);
            }
            led_str = (HAL_GPIO_ReadPin(BOARD_LED_PORT, BOARD_LED_PIN) != GPIO_PIN_RESET)
                          ? "on"
                          : "off";
            key_str = (HAL_GPIO_ReadPin(KEY_PORT, KEY_PIN) != GPIO_PIN_RESET)
                          ? "high"
                          : "low";
            knob_mv = (knob_raw * ADC1_VDDA_MV) / ADC1_FULL_SCALE;
            if (bmp_ok != 0U) {
                temp_centi_cached = BMP280_ReadTemp();
                press_pa = BMP280_ReadPressure();
                temp_abs = (temp_centi_cached < 0)
                               ? (uint32_t)(-temp_centi_cached)
                               : (uint32_t)temp_centi_cached;
                LOG_D("LED %s  KEY %s  knob raw=%u mv=%u duty=%u  clock %02u:%02u:%02u:%02u  temp %s%u.%02u C press %u Pa",
                      led_str, key_str, (unsigned int)knob_raw, (unsigned int)knob_mv,
                      (unsigned int)fan_duty, (unsigned int)clock_h,
                      (unsigned int)clock_m, (unsigned int)clock_s,
                      (unsigned int)clock_cs,
                      (temp_centi_cached < 0) ? "-" : "",
                      (unsigned int)(temp_abs / 100U),
                      (unsigned int)(temp_abs % 100U),
                      (unsigned int)press_pa);
            } else {
                temp_centi_cached = 0;
                LOG_D("LED %s  KEY %s  knob raw=%u mv=%u duty=%u  clock %02u:%02u:%02u:%02u",
                      led_str, key_str, (unsigned int)knob_raw, (unsigned int)knob_mv,
                      (unsigned int)fan_duty, (unsigned int)clock_h,
                      (unsigned int)clock_m, (unsigned int)clock_s,
                      (unsigned int)clock_cs);
            }
        }
    }
}

static uint32_t knob_to_fan_duty(uint32_t knob_raw)
{
    if (knob_raw <= FAN_KNOB_DEADZONE) {
        return 0U;
    }
    return (knob_raw * FAN_DUTY_MAX) / ADC1_FULL_SCALE;
}

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLMUL = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) {
        return;
    }

    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) {
        return;
    }
}

void Error_Handler(void)
{
    __disable_irq();
    while (1) {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif
