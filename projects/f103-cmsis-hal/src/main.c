/**
 * @file    main.c
 * @brief   STM32F103C8T6 核心板 PC13 LED 闪烁（CMSIS + HAL）
 *
 * @target  STM32F103C8T6
 * @note    行为对齐 f103-manual-reg：HSE→72 MHz、Backup 域 PC13、低电平点亮
 *          终端/CLI 输出保持 English（本 demo 无 printf）
 */

#include "main.h"

#define LED_PIN GPIO_PIN_13

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void delay(volatile uint32_t count);

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();

    for (;;) {
        HAL_GPIO_WritePin(GPIOC, LED_PIN, GPIO_PIN_SET);
        delay(0xFFFFFU);
        delay(0xFFFFFU);

        HAL_GPIO_WritePin(GPIOC, LED_PIN, GPIO_PIN_RESET);
        delay(0xFFFFFU);
        delay(0xFFFFFU);
    }
}

/**
 * @brief  HSE 8 MHz × PLL9 → 72 MHz；HSE 超时则保持 HSI（与 manual-reg 一致）
 */
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
        /* HSE 失败：保持复位默认 HSI 8 MHz，不调用 Error_Handler 死循环 */
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

/**
 * @brief  PC13 推挽输出；Backup 域须先使能 PWR 时钟并解除 DBP
 */
static void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef gpio = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    HAL_PWR_EnableBkUpAccess();

    __HAL_RCC_GPIOC_CLK_ENABLE();

    gpio.Pin = LED_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOC, &gpio);
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
