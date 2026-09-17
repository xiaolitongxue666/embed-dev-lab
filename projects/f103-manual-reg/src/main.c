/**
 * @file    main.c
 * @brief   STM32F103C8T6：PC13 / PB12 LED + PB13 KEY + USART1 DMA+IDLE + ADC1 PA0 + SPI1 BMP280 + TIM2 JY003 + I2C1 SH1106
 *
 * @target  STM32F103C8T6（Medium-density，64 KB Flash / 20 KB RAM）
 *
 * 进入本文件之前（见 startup_stm32f103xb.s，勿在此再调 SystemInit）：
 *   复位 → g_pfnVectors → Reset_Handler
 *        → 设 SP → 拷贝 .data → 清零 .bss
 *        → bl SystemInit（system_stm32f1xx.c：HSE×PLL→72 MHz）
 *        → bl main          ← 此处
 *
 * main 内初始化顺序：
 *   1. GPIOC_Init → GPIOB_Init → USART1_Init → 注册 RX 整帧回显 handle
 *      → ADC1_Init → TIM2_PWM_Init → SPI1_Init → I2C1_Init
 *   2. 延时 ≥20 ms（BMP280 / 预留 LSM6 boot）
 *   3. BMP280 ID / Init → Probe 0x78 → SH1106 时钟 + 右下温度
 *   4. 循环：旋钮 raw→风扇占空比 + LED + KEY + USART1_ProcessRx；每秒刷新时钟与温度
 *
 * 串口：USART1 PA9/PA10（FT），1500000 bps；DMA1 CH4/CH5 + 空闲中断定界。
 * printf 经 syscalls.c → USART1_Write；RX 整帧经 handle 回显。
 * SPI1：PA5/PA7/PA6 共用；PA3=BMP280 CSB Mode 0；PA8=LSM6 CS Mode 3（驱动保留，本阶段不访问）。
 *       JY003 PWM=PA1 TIM2_CH2（电机电源独立）。BMP280 SDO 禁止接地。
 *   PA3、PA5–PA7 手册未标 FT；PA8 为 FT；详表见 spi.c 与引脚总表。
 * ADC1：PA0 = ADC12_IN0（非 FT）；DMA1 CH1 循环；映射 TIM2_CCR2。
 * PB13 KEY：EXTI13 双沿。SPI1 多字节走 DMA1 CH2/CH3。
 *       PA0 不作风扇 PWM（TIM2_CH1 已被旋钮占用）。
 * PC13 LED：非 FT；Backup 域，须先 PWREN+DBP；灌电流，低电平点亮。
 * PB12 LED：FT；拉电流，PB12→220 Ω→LED+，LED-→GND；与 PC13 同步翻转、写相同电平。
 * PB13 KEY：FT；上拉输入，PB13←按键→GND；按下 IDR=0，松开 IDR=1。
 * SH1106：I2C1 PB6=SCL / PB7=SDA，8 位写地址 0x78；列偏移 2；中央 HH:MM:SS；右下温度。
 *
 * @see     doc/projects/f103-manual-reg.md § 启动与时钟 / 运行时初始化顺序
 * @see     doc/learn/stm32-bare-metal-bootstrap.md Q11
 * @see     doc/hardware/stm32f103-peripherals.md
 * @see     doc/hardware/stm32f103c8t6-pinout.md
 * @see     doc/learn/gpio-eight-modes.md — PC13 / PB12 推挽；PB13 上拉输入；PA0 模拟
 */

#include <stdio.h>

#include "adc.h"
#include "bmp280.h"
#include "gpioc_bitband.h"
#include "i2c.h"
#include "nvic.h"
#include "sh1106.h"
#include "spi.h"
#include "systick.h"
#include "tim2.h"
#include "usart.h"

/* -------------------------------------------------------------------------- */
/* 外设基地址与寄存器映射（参考 RM0008）                                          */
/* -------------------------------------------------------------------------- */

#define RCC_BASE     0x40021000U
#define RCC_APB2ENR  (*(volatile unsigned int *)(RCC_BASE + 0x18U))
#define RCC_APB1ENR  (*(volatile unsigned int *)(RCC_BASE + 0x1CU))

#define PWR_BASE 0x40007000U
#define PWR_CR   (*(volatile unsigned int *)(PWR_BASE + 0x00U))

#define GPIOC_BASE 0x40011000U
#define GPIOC_CRH  (*(volatile unsigned int *)(GPIOC_BASE + 0x04U))
#define GPIOB_CRH  (*(volatile unsigned int *)(GPIOB_BASE + 0x04U))

#define AFIO_BASE    0x40010000U
#define AFIO_EXTICR4 (*(volatile unsigned int *)(AFIO_BASE + 0x14U))
#define EXTI_BASE    0x40010400U
#define EXTI_IMR     (*(volatile unsigned int *)(EXTI_BASE + 0x00U))
#define EXTI_RTSR    (*(volatile unsigned int *)(EXTI_BASE + 0x08U))
#define EXTI_FTSR    (*(volatile unsigned int *)(EXTI_BASE + 0x0CU))
#define EXTI_PR      (*(volatile unsigned int *)(EXTI_BASE + 0x14U))

#define RCC_APB1ENR_PWREN  (1U << 28)
#define RCC_APB2ENR_AFIOEN (1U << 0)
#define RCC_APB2ENR_IOPBEN (1U << 3)
#define RCC_APB2ENR_IOPCEN (1U << 4)
#define PWR_CR_DBP         (1U << 8)
#define AFIO_EXTICR4_EXTI13_MASK (0xFU << 4)
#define AFIO_EXTICR4_EXTI13_PB   (1U << 4)
#define EXTI_LINE13        (1U << 13)
#define KEY_EXTI_PRIO      7U

#define GPIOC_CRH_PC13_MASK   (0xFU << 20)
#define GPIOC_CRH_PC13_OUT_PP (3U << 20)
#define GPIOB_CRH_PB12_MASK   (0xFU << 16)
#define GPIOB_CRH_PB12_OUT_PP (3U << 16)
#define GPIOB_CRH_PB13_MASK   (0xFU << 20)
#define GPIOB_CRH_PB13_IN_PU  (8U << 20)

#define BOARD_LED_PIN 13U
#define EXT_LED_PIN   12U
#define KEY_PIN       13U

static volatile unsigned char g_key_edge;

/**
 * @brief  软件延时（忙等待）
 * @param  count  递减计数初值，与 CPU 主频相关
 */
static void delay(volatile unsigned int count)
{
    while (count != 0U) {
        count--;
    }
}

/**
 * @brief  约 ≥20 ms 忙等（72 MHz 下经验计数，满足 LSM6DS3 / BMP280 boot）
 *
 * AN4650：上电后约 20 ms 加载 trim，此前勿访问寄存器。
 * 单次 0xFFFFF 循环在 72 MHz 上约数十 ms 量级，调用两次留余量。
 */
static void delay_boot_20ms(void)
{
    delay(0xFFFFFU);
    delay(0xFFFFFU);
}

/**
 * @brief  初始化 PC13 为推挽输出（须先 PWREN + DBP）
 * @see    doc/learn/gpio-eight-modes.md
 */
static void GPIOC_Init(void)
{
    RCC_APB1ENR |= RCC_APB1ENR_PWREN;
    PWR_CR |= PWR_CR_DBP;

    RCC_APB2ENR |= RCC_APB2ENR_IOPCEN;

    GPIOC_CRH &= ~GPIOC_CRH_PC13_MASK;
    GPIOC_CRH |= GPIOC_CRH_PC13_OUT_PP;
}

/**
 * @brief  初始化 PB12 推挽输出 + PB13 上拉输入（无需 Backup 解锁）
 * @see    doc/learn/gpio-eight-modes.md
 */
static void GPIOB_Init(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_IOPBEN;

    GPIOB_CRH &= ~GPIOB_CRH_PB12_MASK;
    GPIOB_CRH |= GPIOB_CRH_PB12_OUT_PP;

    /* CNF=10 MODE=00 → 上拉/下拉输入；ODR=1 选片内上拉 */
    GPIOB_CRH &= ~GPIOB_CRH_PB13_MASK;
    GPIOB_CRH |= GPIOB_CRH_PB13_IN_PU;
    PBout(KEY_PIN) = 1U;
}

/**
 * @brief  PB13 → EXTI13 双沿（RM0008 AFIO_EXTICR4 / EXTI IMR/FTSR/RTSR）
 */
static void Key_ExtiInit(void)
{
    RCC_APB2ENR |= RCC_APB2ENR_AFIOEN;
    AFIO_EXTICR4 = (AFIO_EXTICR4 & ~AFIO_EXTICR4_EXTI13_MASK) |
                   AFIO_EXTICR4_EXTI13_PB;
    EXTI_FTSR |= EXTI_LINE13;
    EXTI_RTSR |= EXTI_LINE13;
    EXTI_IMR |= EXTI_LINE13;
    EXTI_PR = EXTI_LINE13;
    NVIC_IRQ_SetPriority(EXTI15_10_IRQn, KEY_EXTI_PRIO);
    NVIC_IRQ_Enable(EXTI15_10_IRQn);
}

void EXTI15_10_IRQHandler(void)
{
    if ((EXTI_PR & EXTI_LINE13) != 0U) {
        EXTI_PR = EXTI_LINE13;
        g_key_edge = 1U;
    }
}

/**
 * @brief  RX 整帧回显（由 USART1_ProcessRx 在主循环调用，不在 ISR 内）
 */
static void USART1_EchoFrame(const unsigned char *data, unsigned int len)
{
    USART1_Write((const char *)data, (int)len);
}

/**
 * @brief  旋钮 raw（0..4095）映射风扇占空比（0..999），低端死区
 */
static unsigned int knob_to_fan_duty(unsigned int knob_raw)
{
    if (knob_raw <= FAN_KNOB_DEADZONE) {
        return 0U;
    }
    return (knob_raw * FAN_DUTY_MAX) / ADC1_FULL_SCALE;
}

/**
 * @brief  程序入口
 */
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

        if (g_key_edge != 0U) {
            g_key_edge = 0U;
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
                /* CS 接触不良等读失败：右下角仍画 0.00C */
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
            /* 字符串指 GPIO 电平：高=on，低=off；PC13 高=灯灭，PB12 高=灯亮 */
            if (PCout(BOARD_LED_PIN) != 0U) {
                printf("PC13 LED on\n");
                printf("PB12 LED on\n");
            } else {
                printf("PC13 LED off\n");
                printf("PB12 LED off\n");
            }
            /* IDR 电平：high=松开，low=按下（低有效） */
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
