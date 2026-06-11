/**
 * @file    stm32f103c8.ld
 * @brief   STM32F103C8T6 链接脚本
 *
 * @memory 64 KB Flash @ 0x0800_0000，20 KB SRAM @ 0x2000_0000
 * @stack  栈顶 _estack = RAM 末尾（满递减栈）
 *
 * @note    符号 _sidata/_sdata/_edata/_sbss/_ebss 供 startup 初始化段使用
 */

ENTRY(Reset_Handler)

/* Cortex-M3 主栈顶：SRAM 最高地址（栈向下增长） */
_estack = 0x20005000;

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

SECTIONS
{
    /* 中断向量表必须位于 Flash 起始 */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    /* 代码与只读数据 */
    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    /* .data 加载地址（LMA 在 Flash，VMA 在 RAM） */
    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    /* 未初始化全局变量 */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM
}
