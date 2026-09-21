/**
 * @file    usart.c
 * @brief   USART1 初始化与字符串发送（HAL_UART_Transmit）
 *
 * @note    USART1 挂 APB2；SYSCLK 72 MHz 时 PCLK2=72 MHz，1500000 bps 分频整除。
 *          硬件：CH341 RX←PA9，CH341 TX→PA10，GND 共地（与蓝板/面包板/ST-Link 同一地）。
 *          PA9/PA10：DS5319 标 FT；默认映射，勿 USART1_REMAP；勿用 PA13/PA14（SWD）。
 *
 * 与 printf / syscalls 的关系（本工程选型）：
 *   - HAL 不提供 printf；stdout 也不会自动接到 UART。
 *   - 本文件 bypass libc：USART1_WriteStr → HAL_UART_Transmit，无 _write、无 syscalls.c。
 *   - 优点：Flash 小（约 6 KB text vs printf 约 30 KB）；路径与 HAL API 一致。
 *   - 需 printf 时：参考 f103-manual-reg 的 syscalls.c，在 _write 内调 HAL_UART_Transmit。
 *
 * `\n` 自动补 `\r`，适配 Windows 串口助手（同 manual-reg 的 _write 行为）。
 *
 * @see     doc/learn/newlib-nosys-stdio-retarget.md
 * @see     doc/hardware/stm32f103c8t6-pinout.md
 * @see     doc/reference/stm32f103/md/topics/lqfp48-pinout.md
 */

#include "usart.h"

/** 与串口助手/COM 工具一致 */
#define USART1_BAUDRATE 1500000U

UART_HandleTypeDef huart1;
static uint8_t s_usart1_rx_byte;

void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = USART1_BAUDRATE;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }
}

void USART1_StartRxEcho(void)
{
    HAL_NVIC_SetPriority(USART1_IRQn, 5, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
    if (HAL_UART_Receive_IT(&huart1, &s_usart1_rx_byte, 1U) != HAL_OK) {
        Error_Handler();
    }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART1) {
        return;
    }
    (void)HAL_UART_Transmit(&huart1, &s_usart1_rx_byte, 1U, 10U);
    (void)HAL_UART_Receive_IT(&huart1, &s_usart1_rx_byte, 1U);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1) {
        (void)HAL_UART_Receive_IT(&huart1, &s_usart1_rx_byte, 1U);
    }
}

/**
 * @brief  经 HAL 阻塞发送以 '\\0' 结尾的字符串
 * @note   遇 '\\n' 先补 '\\r'；不用 printf 故不经过 _write / libnosys
 */
void USART1_WriteStr(const char *str)
{
    uint8_t cr = (uint8_t)'\r';
    uint8_t ch;

    if (str == NULL) {
        return;
    }

    for (; *str != '\0'; str++) {
        if (*str == '\n') {
            (void)HAL_UART_Transmit(&huart1, &cr, 1U, HAL_MAX_DELAY);
        }
        ch = (uint8_t)*str;
        (void)HAL_UART_Transmit(&huart1, &ch, 1U, HAL_MAX_DELAY);
    }
}

static void usart1_write_digits(uint32_t value, uint32_t min_width)
{
    char buf[10];
    uint32_t n = 0U;

    if (value == 0U) {
        buf[n++] = '0';
    } else {
        while (value != 0U) {
            buf[n++] = (char)('0' + (value % 10U));
            value /= 10U;
        }
    }
    while (n < min_width) {
        buf[n++] = '0';
    }
    while (n > 0U) {
        n--;
        (void)HAL_UART_Transmit(&huart1, (uint8_t *)&buf[n], 1U, HAL_MAX_DELAY);
    }
}

void USART1_WriteHex8(uint8_t value)
{
    static const char hex[] = "0123456789ABCDEF";
    uint8_t ch;

    ch = (uint8_t)hex[(value >> 4) & 0x0FU];
    (void)HAL_UART_Transmit(&huart1, &ch, 1U, HAL_MAX_DELAY);
    ch = (uint8_t)hex[value & 0x0FU];
    (void)HAL_UART_Transmit(&huart1, &ch, 1U, HAL_MAX_DELAY);
}

void USART1_WriteU32(uint32_t value)
{
    usart1_write_digits(value, 1U);
}

void USART1_WriteDec2(uint32_t value)
{
    usart1_write_digits(value % 100U, 2U);
}
