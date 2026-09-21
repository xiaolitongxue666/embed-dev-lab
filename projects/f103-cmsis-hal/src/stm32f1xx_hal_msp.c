/**
 * @file    stm32f1xx_hal_msp.c
 * @brief   HAL MSP（MCU Support Package）回调
 *
 * @note    CubeMX 工程中在此实现外设底层初始化（时钟、GPIO 复用、DMA 等）。
 *          PC13 / CS 在 gpio.c；USART1 / SPI1 引脚与时钟在本文件 MSP。
 *
 * 调用关系（HAL 库内部）：
 *   HAL_Init()        → HAL_MspInit()
 *   HAL_GPIO_Init()   → HAL_GPIO_MspInit()（若需单独使能端口时钟可在此实现）
 *
 * 与 f103-manual-reg 对照：manual-reg 无 MSP 层，寄存器操作集中在 main.c。
 */

#include "adc.h"
#include "main.h"

/** @brief  全局 MSP：HAL 库初始化时调用一次 */
void HAL_MspInit(void)
{
}

/**
 * @brief  单引脚 GPIO MSP；本 demo 时钟已在 MX_GPIO_Init 中使能
 * @param  gpio_init_struct  HAL 传入的引脚配置（本实现未使用）
 */
void HAL_GPIO_MspInit(GPIO_InitTypeDef *gpio_init_struct)
{
    (void)gpio_init_struct;
}

/**
 * @brief  USART1 MSP：PA9 TX（复用推挽）、PA10 RX（浮空输入=高阻）
 * @note   F103 默认映射，无需 AFIO USART1 重映射（remap 会抢 PB6/PB7）
 *         PA9/PA10 为 FT；勿占用 PA13/PA14（SWD）
 * @see    doc/learn/gpio-eight-modes.md
 * @see    doc/learn/uart-ttl-rs232-rs485.md
 * @see    doc/hardware/stm32f103c8t6-pinout.md
 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio = {0};

    if (huart->Instance != USART1) {
        return;
    }

    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_9;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_10;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1) {
        return;
    }

    __HAL_RCC_USART1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9 | GPIO_PIN_10);
}

/**
 * @brief  SPI1 MSP：PA5 SCK / PA7 MOSI 复用推挽，PA6 MISO 浮空输入
 * @note   默认映射；CS 为 GPIO（PA3/PA8），不在本函数
 */
void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef gpio = {0};

    if (hspi->Instance != SPI1) {
        return;
    }

    __HAL_RCC_SPI1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_5 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    gpio.Pin = GPIO_PIN_6;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);
}

void HAL_SPI_MspDeInit(SPI_HandleTypeDef *hspi)
{
    if (hspi->Instance != SPI1) {
        return;
    }

    __HAL_RCC_SPI1_CLK_DISABLE();
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7);
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    GPIO_InitTypeDef gpio = {0};

    if (hadc->Instance != ADC1) {
        return;
    }

    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_ADC_CONFIG(RCC_ADCPCLK2_DIV6);

    gpio.Pin = GPIO_PIN_0;
    gpio.Mode = GPIO_MODE_ANALOG;
    HAL_GPIO_Init(GPIOA, &gpio);

    hdma_adc1.Instance = DMA1_Channel1;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_DISABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_MEDIUM;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) {
        Error_Handler();
    }
    __HAL_LINKDMA(hadc, DMA_Handle, hdma_adc1);
    HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

void HAL_TIM_PWM_MspInit(TIM_HandleTypeDef *htim)
{
    GPIO_InitTypeDef gpio = {0};

    if (htim->Instance != TIM2) {
        return;
    }

    __HAL_RCC_TIM2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_1;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);
}

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef gpio = {0};

    if (hi2c->Instance != I2C1) {
        return;
    }

    __HAL_RCC_I2C1_CLK_ENABLE();
    __HAL_RCC_I2C1_FORCE_RESET();
    __HAL_RCC_I2C1_RELEASE_RESET();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    gpio.Pin = GPIO_PIN_6 | GPIO_PIN_7;
    gpio.Mode = GPIO_MODE_AF_OD;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &gpio);
}
