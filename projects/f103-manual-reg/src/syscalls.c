/**
 * @file    syscalls.c
 * @brief   newlib 底层 I/O 钩子：printf 重定向到 USART1
 *
 * 调用链（裸机无操作系统）：
 *   printf / vfprintf — 工具链 newlib（libc.a），不在本仓库
 *   stdio 写 stdout   — 同上；libc 内部 _write_r() 再调 _write()
 *   _write            — 本文件
 *   USART1_Write      — usart.c
 *
 * 与 libnosys 的关系（链接期替换，非 weak 覆盖）：
 *   工具链设 --specs=nosys.specs（见 cmake/toolchain-arm-none-eabi.cmake），会链入 libnosys.a，
 *   其中自带 _write / _sbrk 等默认桩（nm 显示为 T 强符号，不是 W weak）。
 *   本文件提供同名强符号；链接时 syscalls.c.obj 先满足引用，libnosys.a 中同名成员不再被拉入。
 *   效果：用 USART 实现替换 nosys 占位桩；机制是链接期符号解析，不是 __attribute__((weak)) 覆盖。
 *
 * 本文件实现的 syscall 接口：
 *   - _write  — 标准输出/调试打印（本工程唯一实际使用的路径）
 *   - _sbrk   — 堆扩展（printf 格式化可能 malloc；同上，链接期替换 libnosys 默认 _sbrk）
 *   - _read/_close/_fstat/_isatty/_lseek — 最小桩，满足链接与 isatty 检测
 *
 * 堆与链接脚本（STM32F103C8_FLASH.ld）：
 *   _ebss/end — .bss 结束、堆区起点（与 syscalls 中 heap_ptr 初值同址）
 *   _estack   — 主栈顶（0x20005000）；堆向上增长，栈向下增长，中间留 512 字节余量
 *
 * 换行：Windows 串口助手认 CRLF；C 字符串 `\n` 在 _write 中自动前置 `\r`。
 *
 * @see     usart.c — USART1 寄存器初始化与字节发送
 * @see     linker/STM32F103C8_FLASH.ld — end / _end / _estack 符号
 * @see     doc/projects/f103-manual-reg.md — § printf 与 newlib syscall
 * @see     doc/learn/newlib-nosys-stdio-retarget.md — nosys、_write 与 HAL 分工
 */

#include <errno.h>
#include <stddef.h>
#include <sys/stat.h>
#include <unistd.h>

#include "usart.h"

/** 链接脚本 .bss 段末尾符号；堆从此地址向上增长 */
extern char _ebss;
/** 链接脚本主栈顶（Cortex-M 满递减栈 SP 初值） */
extern char _estack;

/** 当前堆分配指针；初值 = &_ebss，与 ld 中 PROVIDE(end) 同址 */
static char *heap_ptr = &_ebss;

/**
 * @brief  扩展堆空间（newlib malloc/printf 内部可能调用）
 * @param  incr  本次请求的堆增量（字节）；负值不支持
 * @return 扩展前堆顶指针；失败返回 (void *)-1 并置 errno=ENOMEM
 *
 * 与 libnosys.a 中默认 _sbrk 同名；本实现为强符号，链接时替换 nosys 桩（非 weak 覆盖）。
 * 以 _ebss 为堆起点，上限距 _estack 保留 512 字节，避免堆栈相撞。
 */
void *_sbrk(ptrdiff_t incr)
{
    char *prev;

    if (incr < 0) {
        errno = ENOMEM;
        return (void *)-1;
    }

    prev = heap_ptr;
    heap_ptr += incr;

    if (heap_ptr > &_estack - 512) {
        heap_ptr = prev;
        errno = ENOMEM;
        return (void *)-1;
    }

    return prev;
}

/**
 * @brief  低级写接口；libc 经 _write_r() 最终调用此处
 * @param  fd   文件描述符（裸机忽略，printf 通常为 1）
 * @param  ptr  待写缓冲区
 * @param  len  字节数
 * @return 成功写入的字节数（与 len 相同）；本实现不返回负 errno 路径
 *
 * 链接：与 libnosys.a 默认 _write 同名强符号，链接期由本函数替换 nosys 占位桩。
 * 逐字节发送；遇 `\n` 先补 `\r`，使 Windows 串口终端正确换行到行首。
 */
int _write(int fd, char *ptr, int len)
{
    int i;
    char cr = '\r';

    (void)fd;
    if (len <= 0) {
        return 0;
    }

    for (i = 0; i < len; i++) {
        if (ptr[i] == '\n') {
            USART1_Write(&cr, 1);
        }
        USART1_Write(&ptr[i], 1);
    }
    return len;
}

/**
 * @brief  读桩：无 stdin 设备
 * @note   printf 单向输出不需要；保留以满足 newlib 链接
 */
int _read(int fd, char *ptr, int len)
{
    (void)fd;
    (void)ptr;
    (void)len;
    errno = EIO;
    return -1;
}

/** @brief  关闭文件桩；裸机无文件系统 */
int _close(int fd)
{
    (void)fd;
    errno = EIO;
    return -1;
}

/**
 * @brief  文件状态桩；声明 fd 为字符设备（TTY）
 * @note   libc 据此判断 stdout 是否为终端，影响缓冲策略
 */
int _fstat(int fd, struct stat *st)
{
    (void)fd;
    st->st_mode = S_IFCHR;
    return 0;
}

/** @brief  是否 TTY；返回 1 使 printf 行缓冲行为符合终端预期 */
int _isatty(int fd)
{
    (void)fd;
    return 1;
}

/** @brief  _seek 桩；串口不可定位 */
int _lseek(int fd, int ptr, int dir)
{
    (void)fd;
    (void)ptr;
    (void)dir;
    errno = EIO;
    return -1;
}
