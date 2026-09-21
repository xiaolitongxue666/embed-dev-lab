/**
 * @file    main.c
 * @brief   CMSIS + HAL demo：LED、USART1、ADC、风扇、I2C OLED、SPI BMP280
 *
 * 主循环不访问 LSM6。BMP280 失败时 OLED 画 0.00C。
 * 串口用 USART1_WriteStr（无 printf）。
 */

#include "adc.h"
#include "bmp280.h"
#include "gpio.h"
#include "i2c.h"
#include "key.h"
#include "lsm6ds3.h"
#include "main.h"
#include "sh1106.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"

/* 保留 LSM6 驱动进 ELF（--gc-sections）；main 运行时不调用 */
static void (*const s_lsm6ds3_keep)(void) = LSM6DS3_Init;

/* OLED 轮询刷新会拉长循环；LED/旋钮日志按 tick，约对齐 manual-reg 200 圈 */
#define LED_LOG_PERIOD_MS 1000U

static void SystemClock_Config(void);
static void delay(volatile uint32_t count);
static void log_temp_press(int32_t temp_centi, uint32_t press_pa);
static uint32_t knob_to_fan_duty(uint32_t knob_raw);

int main(void)
{
    uint8_t bmp_id;
    uint8_t bmp_ok;
    uint8_t oled_ok;
    uint32_t last_led_ms;
    uint32_t now_ms;
    uint32_t knob_raw;
    uint32_t knob_mv;
    uint32_t fan_duty;
    uint32_t last_sec;
    uint32_t elapsed_sec;
    uint32_t clock_h;
    uint32_t clock_m;
    uint32_t clock_s;
    int32_t temp_centi;
    uint32_t press_pa;
    GPIO_PinState led_level;

    HAL_Init();
    (void)s_lsm6ds3_keep;
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();
    USART1_WriteStr("Stm32 cmsis hal BMP280 + JY003 fan + SH1106 I2C demo start\n");
    USART1_WriteStr("LSM6DS3 deferred (driver linked, main unused)\n");

    Key_ExtiInit();
    MX_ADC1_Init();
    MX_TIM2_Init();
    MX_SPI1_Init();
    MX_I2C1_Init();
    USART1_StartRxEcho();

    HAL_Delay(20U);

    bmp_id = BMP280_ReadID();
    USART1_WriteStr("BMP280 ID=0x");
    USART1_WriteHex8(bmp_id);
    USART1_WriteStr(" (expect 0x");
    USART1_WriteHex8(BMP280_ID_VALUE);
    USART1_WriteStr(" / BME 0x");
    USART1_WriteHex8(BME280_ID_VALUE);
    USART1_WriteStr(")\n");
    if ((bmp_id != BMP280_ID_VALUE) && (bmp_id != BME280_ID_VALUE)) {
        USART1_WriteStr("BMP280 ID mismatch; check CSB/SDO/Mode0/3V3/GND\n");
    }

    bmp_ok = BMP280_Init();
    if (bmp_ok != 0U) {
        USART1_WriteStr("BMP280 init: calib + normal, osrs_t x2 / osrs_p x16\n");
    } else {
        USART1_WriteStr("BMP280 init failed\n");
    }

    oled_ok = 0U;
    if (I2C1_Probe(SH1106_ADDR_WR) != 0U) {
        USART1_WriteStr("SH1106 ACK addr=0x78\n");
        SH1106_Init();
        oled_ok = 1U;
    } else {
        USART1_WriteStr("SH1106 NACK; check PB6/PB7/3V3/GND\n");
    }

    last_led_ms = HAL_GetTick();
    last_sec = 0xFFFFFFFFU;
    for (;;) {
        if (Key_TakeEdge() != 0U) {
            if (HAL_GPIO_ReadPin(KEY_PORT, KEY_PIN) != GPIO_PIN_RESET) {
                USART1_WriteStr("PB13 KEY irq high\n");
            } else {
                USART1_WriteStr("PB13 KEY irq low\n");
            }
        }

        knob_raw = ADC1_ReadRaw();
        fan_duty = knob_to_fan_duty(knob_raw);
        Fan_SetDuty(fan_duty);

        elapsed_sec = HAL_GetTick() / 1000U;
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
                log_temp_press(temp_centi, press_pa);
            } else {
                SH1106_DrawTemp(0);
            }
            SH1106_Refresh();
            USART1_WriteStr("clock ");
            USART1_WriteDec2(clock_h);
            USART1_WriteStr(":");
            USART1_WriteDec2(clock_m);
            USART1_WriteStr(":");
            USART1_WriteDec2(clock_s);
            USART1_WriteStr("\n");
        }

        now_ms = HAL_GetTick();
        if ((now_ms - last_led_ms) >= LED_LOG_PERIOD_MS) {
            last_led_ms = now_ms;
            led_level = HAL_GPIO_ReadPin(BOARD_LED_PORT, BOARD_LED_PIN);
            if (led_level != GPIO_PIN_RESET) {
                HAL_GPIO_WritePin(BOARD_LED_PORT, BOARD_LED_PIN, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(EXT_LED_PORT, EXT_LED_PIN, GPIO_PIN_RESET);
                USART1_WriteStr("PC13 LED off\n");
                USART1_WriteStr("PB12 LED off\n");
            } else {
                HAL_GPIO_WritePin(BOARD_LED_PORT, BOARD_LED_PIN, GPIO_PIN_SET);
                HAL_GPIO_WritePin(EXT_LED_PORT, EXT_LED_PIN, GPIO_PIN_SET);
                USART1_WriteStr("PC13 LED on\n");
                USART1_WriteStr("PB12 LED on\n");
            }
            if (HAL_GPIO_ReadPin(KEY_PORT, KEY_PIN) != GPIO_PIN_RESET) {
                USART1_WriteStr("PB13 KEY high\n");
            } else {
                USART1_WriteStr("PB13 KEY low\n");
            }
            knob_mv = (knob_raw * ADC1_VDDA_MV) / ADC1_FULL_SCALE;
            USART1_WriteStr("knob raw=");
            USART1_WriteU32(knob_raw);
            USART1_WriteStr(" mv=");
            USART1_WriteU32(knob_mv);
            USART1_WriteStr(" duty=");
            USART1_WriteU32(fan_duty);
            USART1_WriteStr("\n");
            if (bmp_ok != 0U) {
                temp_centi = BMP280_ReadTemp();
                press_pa = BMP280_ReadPressure();
                log_temp_press(temp_centi, press_pa);
            }
        }

        delay(0x7FFFU);
    }
}

static void log_temp_press(int32_t temp_centi, uint32_t press_pa)
{
    uint32_t temp_abs;

    USART1_WriteStr("temp ");
    if (temp_centi < 0) {
        USART1_WriteStr("-");
        temp_abs = (uint32_t)(-temp_centi);
    } else {
        temp_abs = (uint32_t)temp_centi;
    }
    USART1_WriteU32(temp_abs / 100U);
    USART1_WriteStr(".");
    USART1_WriteDec2(temp_abs % 100U);
    USART1_WriteStr(" C  press ");
    USART1_WriteU32(press_pa);
    USART1_WriteStr(" Pa\n");
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

static void delay(volatile uint32_t count)
{
    while (count != 0U) {
        count--;
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