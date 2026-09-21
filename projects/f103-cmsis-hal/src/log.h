/**
 * @file    log.h
 * @brief   分级日志：秒级时间戳 + 级别，经 HAL_UART_Transmit
 *
 * 行格式：[HH:MM:SS][I] message
 * 禁止在 ISR 中调用。不使用 printf / syscalls。
 *
 * @see     log.c
 */

#ifndef LOG_H
#define LOG_H

enum {
    LOG_ERROR = 0,
    LOG_WARN,
    LOG_INFO,
    LOG_DEBUG
};

#ifndef LOG_COMPILE_LEVEL
#define LOG_COMPILE_LEVEL LOG_DEBUG
#endif

#ifndef LOG_COLOR
#define LOG_COLOR 1
#endif

void log_write(unsigned int level, const char *fmt, ...);

#define LOG_E(...) log_write((unsigned int)LOG_ERROR, __VA_ARGS__)
#define LOG_W(...) log_write((unsigned int)LOG_WARN, __VA_ARGS__)
#define LOG_I(...) log_write((unsigned int)LOG_INFO, __VA_ARGS__)
#define LOG_D(...) log_write((unsigned int)LOG_DEBUG, __VA_ARGS__)

#endif /* LOG_H */
