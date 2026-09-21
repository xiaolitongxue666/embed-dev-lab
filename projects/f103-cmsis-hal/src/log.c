/**
 * @file    log.c
 * @brief   日志：SysTime 秒级前缀、可选 ANSI、一次 HAL_UART_Transmit
 *
 * 不走 USART1_WriteStr 逐字节，不链 syscalls。
 *
 * @see     log.h
 */

#include "log.h"

#include <stdarg.h>
#include <stdio.h>

#include "timer_event.h"
#include "usart.h"

#define LOG_BODY_MAX 160
#define LOG_LINE_MAX 200

#if LOG_COLOR
#define LOG_ANSI_RESET "\033[0m"
#define LOG_ANSI_RED   "\033[31m"
#define LOG_ANSI_YEL   "\033[33m"
#define LOG_ANSI_GRN   "\033[32m"
#define LOG_ANSI_CYN   "\033[36m"
#endif

void log_write(unsigned int level, const char *fmt, ...)
{
    va_list args;
    SysTime_t now;
    uint32_t clock_h;
    uint32_t clock_m;
    uint32_t clock_s;
    const char *letter;
    char body[LOG_BODY_MAX];
    char line[LOG_LINE_MAX];
    int n;
#if LOG_COLOR
    const char *color;
#endif

    if ((fmt == NULL) || (level > (unsigned int)LOG_COMPILE_LEVEL)) {
        return;
    }

    SysTime_Get(&now);
    clock_h = (now.sec / 3600U) % 100U;
    clock_m = (now.sec / 60U) % 60U;
    clock_s = now.sec % 60U;

    switch (level) {
    case (unsigned int)LOG_ERROR:
        letter = "E";
#if LOG_COLOR
        color = LOG_ANSI_RED;
#endif
        break;
    case (unsigned int)LOG_WARN:
        letter = "W";
#if LOG_COLOR
        color = LOG_ANSI_YEL;
#endif
        break;
    case (unsigned int)LOG_INFO:
        letter = "I";
#if LOG_COLOR
        color = LOG_ANSI_GRN;
#endif
        break;
    default:
        letter = "D";
#if LOG_COLOR
        color = LOG_ANSI_CYN;
#endif
        break;
    }

    va_start(args, fmt);
    (void)vsnprintf(body, (size_t)LOG_BODY_MAX, fmt, args);
    va_end(args);

#if LOG_COLOR
    n = snprintf(line, (size_t)LOG_LINE_MAX, "%s[%02lu:%02lu:%02lu][%s] %s%s\r\n",
                 color, (unsigned long)clock_h, (unsigned long)clock_m,
                 (unsigned long)clock_s, letter, body, LOG_ANSI_RESET);
#else
    n = snprintf(line, (size_t)LOG_LINE_MAX, "[%02lu:%02lu:%02lu][%s] %s\r\n",
                 (unsigned long)clock_h, (unsigned long)clock_m,
                 (unsigned long)clock_s, letter, body);
#endif

    if (n <= 0) {
        return;
    }
    if (n >= LOG_LINE_MAX) {
        n = LOG_LINE_MAX - 1;
    }
    (void)HAL_UART_Transmit(&huart1, (uint8_t *)line, (uint16_t)n, HAL_MAX_DELAY);
}
