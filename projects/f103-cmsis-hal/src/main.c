/**
 * @file    main.c
 * @brief   STM32F103C8T6 核心板 PC13 LED 闪烁（CMSIS + HAL）
 *
 * @target  STM32F103C8T6（Medium-density F103xB，64 KB Flash / 20 KB RAM）
 * @note    行为对齐 f103-manual-reg：HSE→72 MHz、Backup 域 PC13、低电平点亮
 *          USART1（PA9/PA10）1500000 bps；串口用 HAL_UART_Transmit（usart.c），不用 printf
 *
 * 启动与初始化顺序（Reset 后）：
 *   1. startup：SystemInit → 拷贝 .data → 清零 .bss
 *   2. main：HAL_Init → SystemClock_Config → MX_GPIO_Init → MX_USART1_UART_Init → 闪烁循环
 *
 * HAL 与 manual-reg 对照：
 *   | 步骤           | manual-reg              | 本文件（HAL）                    |
 *   |----------------|-------------------------|----------------------------------|
 *   | 系统时钟       | 手写 RCC 寄存器         | HAL_RCC_OscConfig/ClockConfig    |
 *   | Backup 域 PC13 | PWREN + DBP + GPIOC_CRH | HAL_PWR + HAL_GPIO_Init          |
 *   | LED 翻转       | PCout 位带              | HAL_GPIO_WritePin                |
 *   | 串口输出       | printf → syscalls       | USART1_WriteStr → HAL_UART_Transmit|
 *
 * @see     doc/projects/f103-cmsis-hal.md
 * @see     doc/learn/newlib-nosys-stdio-retarget.md — HAL 与 printf 分层、为何本工程不用 _write
 * @see     doc/reference/stm32f103/md/topics/backup-domain-pc13.md
 */

#include "main.h"
#include "usart.h"

/** PC13 对应 HAL 引脚宏（GPIOC Pin 13） */
#define LED_PIN GPIO_PIN_13

static void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void delay(volatile uint32_t count);

/**
 * @brief  应用入口
 * @note   HAL_Init 配置 SysTick 等 HAL 内部状态；本 demo 未用 HAL_Delay，闪烁用忙等
 */
int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART1_UART_Init();

    USART1_WriteStr("Stm32 cmsis hal demo start\n");

    /* 多数 C8 核心板 PC13 LED 低电平点亮：SET=灭，RESET=亮 */
    for (;;) {
        USART1_WriteStr("LED on\n");
        HAL_GPIO_WritePin(GPIOC, LED_PIN, GPIO_PIN_SET);
        delay(0xFFFFFU);
        delay(0xFFFFFU);

        USART1_WriteStr("LED off\n");
        HAL_GPIO_WritePin(GPIOC, LED_PIN, GPIO_PIN_RESET);
        delay(0xFFFFFU);
        delay(0xFFFFFU);
    }
}

/**
 * @brief  配置系统时钟：HSE 8 MHz × PLL×9 → SYSCLK 72 MHz
 *
 * 时钟树（成功路径）：
 *   HSE(8M) → PLL ×9 → SYSCLK 72 MHz
 *   AHB  = SYSCLK / 1 = 72 MHz（HCLK）
 *   APB1 = HCLK / 2 = 36 MHz（PWR、部分定时器等）
 *   APB2 = HCLK / 1 = 72 MHz（GPIOC 等）
 *
 * Flash 等待周期须与 SYSCLK 匹配：72 MHz 时 LATENCY_2（见 RM0008）。
 * HSE 启动失败时保持复位默认 HSI 8 MHz，LED 仍可能闪烁但频率不同（与 manual-reg 一致）。
 */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    RCC_ClkInitTypeDef clk = {0};

    /* --- 振荡器与 PLL --- */
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

    /* --- 总线分频与 SYSCLK 源 --- */
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
                  | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;   /* APB1 最高 36 MHz */
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) {
        return;
    }
}

/**
 * @brief  初始化 PC13 为推挽输出
 *
 * PC13 属于 Backup 域，须先于 GPIO 配置：
 *   1. __HAL_RCC_PWR_CLK_ENABLE() — 开启 PWR 模块时钟（APB1）
 *   2. HAL_PWR_EnableBkUpAccess() — 置位 PWR_CR.DBP，解除 Backup 域写保护
 *   3. __HAL_RCC_GPIOC_CLK_ENABLE() — 开启 GPIOC 时钟（APB2）
 *   4. HAL_GPIO_Init — 推挽输出、高速
 *
 * 若跳过步骤 1–2，GPIOC 配置写入无效，LED 不亮（与 manual-reg 现象一致）。
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

/** @brief  简单忙等延时；count 越大延时越长，与 CPU 频率相关（无 SysTick 依赖） */
static void delay(volatile uint32_t count)
{
    while (count != 0U) {
        count--;
    }
}

/**
 * @brief  HAL 错误钩子；本 demo 仅在 USE_FULL_ASSERT 时可能间接调用
 * @note   关闭全局中断并死循环，便于调试器检查调用栈
 */
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
