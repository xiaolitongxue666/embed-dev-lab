/**
 * @file    main.c
 * @brief   手写调度 demo：同一套外设，主循环拆成五个任务
 *
 * 初始化顺序与 f103-manual-reg 相同。周期不再走 timer_event：
 *   key 5 ms、串口回显 1 ms、旋钮风扇 1 ms、OLED 50 ms、LED/BMP280 日志 1 s。
 * 调度器启动前 SysTick 只加毫秒，BMP280 忙等和 OLED 清零都靠它。
 *
 * @see     doc/projects/f103-manual-rtos.md
 * @see     doc/learn/manual-rtos-context-switch.md
 */

#include "adc.h"
#include "bmp280.h"
#include "gpio.h"
#include "gpioc_bitband.h"
#include "i2c.h"
#include "key.h"
#include "log.h"
#include "lsm6ds3.h"
#include "nvic.h"
#include "os.h"
#include "sh1106.h"
#include "spi.h"
#include "systick.h"
#include "tim2.h"
#include "usart.h"

#define TASK_PRIO_KEY 4U
#define TASK_PRIO_USART 4U
#define TASK_PRIO_FAN 3U
#define TASK_PRIO_DISPLAY 2U
#define TASK_PRIO_STATUS 1U

#define TASK_KEY_MS 5U
#define TASK_USART_MS 1U
#define TASK_FAN_MS 1U
#define TASK_DISPLAY_MS 50U
#define TASK_STATUS_MS 1000U

/** RCC_CFGR。SWS 在 [3:2]，PLL 为 10b */
#define RCC_CFGR_ADDR 0x40021004U
#define RCC_CFGR_SWS_MASK 0x0CU
#define RCC_CFGR_SWS_PLL 0x08U

static volatile int s_temp_centi;
static volatile unsigned int s_knob_raw;
static volatile unsigned int s_fan_duty;
static volatile unsigned char s_bmp_ok;
static volatile unsigned char s_oled_ok;

/* 保留 LSM6 驱动进 ELF（--gc-sections）；任务里不调用 */
static void (*const s_lsm6ds3_keep)(void) = LSM6DS3_Init;

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

static void log_if_sysclk_not_pll(void)
{
    unsigned int cfgr;

    cfgr = *(volatile unsigned int *)RCC_CFGR_ADDR;
    if ((cfgr & RCC_CFGR_SWS_MASK) != RCC_CFGR_SWS_PLL) {
        LOG_E("SYSCLK is not PLL; SysTick reload 71999 is not 1 ms");
    }
}

static void clock_fields(unsigned int *hour, unsigned int *minute,
                         unsigned int *second, unsigned int *centisecond)
{
    SysTime_t now;

    SysTime_Get(&now);
    *hour = (now.sec / 3600U) % 100U;
    *minute = (now.sec / 60U) % 60U;
    *second = now.sec % 60U;
    *centisecond = now.ms / 10U;
}

static void task_key(void)
{
    while (1) {
        if (Key_TakeEdge() != 0U) {
            if (PBin(KEY_PIN) != 0U) {
                LOG_I("PB13 KEY irq high");
            } else {
                LOG_I("PB13 KEY irq low");
            }
        }
        os_delay(TASK_KEY_MS);
    }
}

static void task_usart(void)
{
    while (1) {
        USART1_ProcessRx();
        os_delay(TASK_USART_MS);
    }
}

static void task_fan(void)
{
    while (1) {
        unsigned int knob_raw;
        unsigned int fan_duty;

        knob_raw = ADC1_ReadRaw();
        fan_duty = knob_to_fan_duty(knob_raw);
        Fan_SetDuty(fan_duty);
        s_knob_raw = knob_raw;
        s_fan_duty = fan_duty;
        os_delay(TASK_FAN_MS);
    }
}

static void task_display(void)
{
    while (1) {
        unsigned int hour;
        unsigned int minute;
        unsigned int second;
        unsigned int centisecond;

        if (s_oled_ok != 0U) {
            clock_fields(&hour, &minute, &second, &centisecond);
            SH1106_Clear();
            SH1106_DrawClock(hour, minute, second, centisecond);
            SH1106_DrawTemp(s_temp_centi);
            SH1106_Refresh();
        }
        os_delay(TASK_DISPLAY_MS);
    }
}

static void task_status(void)
{
    while (1) {
        unsigned int hour;
        unsigned int minute;
        unsigned int second;
        unsigned int centisecond;
        unsigned int knob_raw;
        unsigned int knob_mv;
        unsigned int fan_duty;
        unsigned int press_pa;
        unsigned int temp_abs;
        int temp_centi;
        const char *led_str;
        const char *key_str;

        PCout(BOARD_LED_PIN) ^= 1U;
        PBout(EXT_LED_PIN) ^= 1U;
        led_str = (PCout(BOARD_LED_PIN) != 0U) ? "on" : "off";
        key_str = (PBin(KEY_PIN) != 0U) ? "high" : "low";
        knob_raw = s_knob_raw;
        fan_duty = s_fan_duty;
        knob_mv = (knob_raw * ADC1_VDDA_MV) / ADC1_FULL_SCALE;
        clock_fields(&hour, &minute, &second, &centisecond);

        if (s_bmp_ok != 0U) {
            temp_centi = BMP280_ReadTemp();
            press_pa = BMP280_ReadPressure();
            s_temp_centi = temp_centi;
            temp_abs = (temp_centi < 0) ? (unsigned int)(-temp_centi)
                                        : (unsigned int)temp_centi;
            LOG_D("LED %s  KEY %s  knob raw=%u mv=%u duty=%u  clock %02u:%02u:%02u:%02u  temp %s%u.%02u C press %u Pa",
                  led_str, key_str, knob_raw, knob_mv, fan_duty,
                  hour, minute, second, centisecond,
                  (temp_centi < 0) ? "-" : "",
                  temp_abs / 100U, temp_abs % 100U, press_pa);
        } else {
            s_temp_centi = 0;
            LOG_D("LED %s  KEY %s  knob raw=%u mv=%u duty=%u  clock %02u:%02u:%02u:%02u",
                  led_str, key_str, knob_raw, knob_mv, fan_duty,
                  hour, minute, second, centisecond);
        }
        os_delay(TASK_STATUS_MS);
    }
}

static void create_task(os_task_fn entry, unsigned int priority, const char *name)
{
    if (task_create(entry, priority) == 0) {
        LOG_E("task_create failed: %s", name);
    }
}

int main(void)
{
    unsigned char bmp_id;
    unsigned char bmp_ok;

    (void)s_lsm6ds3_keep;

    /* 按键和串口初始化会写 NVIC IP，分组必须先设成 4 位全抢占 */
    NVIC_SetPriorityGroup4();

    GPIOC_Init();
    GPIOB_Init();
    Key_ExtiInit();
    USART1_Init();
    USART1_SetRxHandle(USART1_EchoFrame);
    log_if_sysclk_not_pll();
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
    s_bmp_ok = bmp_ok;
    if (bmp_ok != 0U) {
        LOG_I("BMP280 init: calib + normal, osrs_t x2 / osrs_p x16");
    } else {
        LOG_E("BMP280 init failed");
    }

    s_oled_ok = 0U;
    s_temp_centi = 0;
    if (I2C1_Probe(SH1106_ADDR_WR) != 0U) {
        LOG_I("SH1106 ACK addr=0x78");
        SH1106_Init();
        SysTick_SetMs(0U);
        s_oled_ok = 1U;
    } else {
        LOG_E("SH1106 NACK; check PB6/PB7/3V3/GND");
    }

    os_init();
    create_task(task_key, TASK_PRIO_KEY, "key");
    create_task(task_usart, TASK_PRIO_USART, "usart");
    create_task(task_fan, TASK_PRIO_FAN, "fan");
    create_task(task_display, TASK_PRIO_DISPLAY, "display");
    create_task(task_status, TASK_PRIO_STATUS, "status");
    os_start();

    while (1) {
    }
}
