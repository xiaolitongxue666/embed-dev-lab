/**
 * @file    log.c
 * @brief   日志：SysTime 秒级前缀、可选 ANSI、一次 printf 出整行
 *
 * 级别高于 LOG_COMPILE_LEVEL 则返回。不进 ISR。
 * 正文先 vsnprintf，再与前缀一次 printf，避免一行三次 USART DMA 等待。
 *
 * @see     log.h
 */

#include "log.h"

#include <stdarg.h>
#include <stdio.h>

#include "systick.h"

#define LOG_BODY_MAX 160

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
    unsigned int clock_h;
    unsigned int clock_m;
    unsigned int clock_s;
    const char *letter;
    char body[LOG_BODY_MAX];
#if LOG_COLOR
    const char *color;
#endif

    if ((fmt == 0) || (level > (unsigned int)LOG_COMPILE_LEVEL)) {
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
    (void)vsnprintf(body, (unsigned int)LOG_BODY_MAX, fmt, args);
    va_end(args);

#if LOG_COLOR
    printf("%s[%02u:%02u:%02u][%s] %s%s\n",
           color, clock_h, clock_m, clock_s, letter, body, LOG_ANSI_RESET);
#else
    printf("[%02u:%02u:%02u][%s] %s\n",
           clock_h, clock_m, clock_s, letter, body);
#endif
}
