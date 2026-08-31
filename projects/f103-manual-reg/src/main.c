/**
 * @file    main.c
 * @brief   STM32F103C8T6：PC13 LED + USART1 printf + SPI1 LSM6DS3 六轴轮询
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
 *   1. GPIOC_Init → USART1_Init → SPI1_Init
 *   2. 延时 ≥20 ms（LSM6DS3 boot）
 *   3. WHO_AM_I → LSM6DS3_Init → 循环读 raw + LED
 *
 * 串口：USART1 PA9/PA10，1500000 bps；printf 经 syscalls.c → usart.c。
 * SPI / LSM6DS3（模块丝印）：
 *   3V3/GND；SCL←PA5，SDA←PA7，SAO→PA6，CS←PA4；Mode 3。
 *   详表见 spi.c 头注释与 doc/hardware/stm32f103-peripherals.md。
 *
 * @see     doc/projects/f103-manual-reg.md § 启动与时钟 / 运行时初始化顺序
 * @see     doc/learn/stm32-bare-metal-bootstrap.md Q11
 * @see     doc/hardware/stm32f103-peripherals.md
 */

#include <stdio.h>

#include "gpioc_bitband.h"
#include "lsm6ds3.h"
#include "spi.h"
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

#define RCC_APB1ENR_PWREN  (1U << 28)
#define RCC_APB2ENR_IOPCEN (1U << 4)
#define PWR_CR_DBP         (1U << 8)

#define GPIOC_CRH_PC13_MASK   (0xFU << 20)
#define GPIOC_CRH_PC13_OUT_PP (3U << 20)

#define LED_PIN 13U

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
 * @brief  约 ≥20 ms 忙等（72 MHz 下经验计数，满足 LSM6DS3 boot）
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
 * @brief  程序入口
 */
int main(void)
{
    unsigned char who;
    LSM6DS3_RawSample sample;
    unsigned int led_phase;

    GPIOC_Init();
    USART1_Init();
    SPI1_Init();

    printf("Stm32 manual reg LSM6DS3 SPI demo start\n");

    delay_boot_20ms();

    who = LSM6DS3_ReadWhoAmI();
    printf("WHO_AM_I=0x%02X (expect 0x%02X)\n",
           (unsigned int)who, (unsigned int)LSM6DS3_WHO_AM_I_VALUE);

    if (who != LSM6DS3_WHO_AM_I_VALUE) {
        printf("LSM6DS3 ID mismatch; check CS/3V3/MISO wiring\n");
    }

    LSM6DS3_Init();
    printf("LSM6DS3 init: XL/G 104 Hz, FS +/-2g / 250 dps\n");

    led_phase = 0U;
    for (;;) {
        if (LSM6DS3_ReadRaw(&sample) != 0U) {
            printf("xl %d %d %d  g %d %d %d\n",
                   (int)sample.ax, (int)sample.ay, (int)sample.az,
                   (int)sample.gx, (int)sample.gy, (int)sample.gz);
        }

        /* 降低 LED 翻转频率，避免刷屏过快掩盖 IMU 行 */
        led_phase++;
        if (led_phase >= 200U) {
            led_phase = 0U;
            PCout(LED_PIN) ^= 1U;
        }

        delay(0x7FFFU);
    }
}
