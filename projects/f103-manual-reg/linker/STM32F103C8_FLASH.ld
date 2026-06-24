/**
 * @file    STM32F103C8_FLASH.ld
 * @brief   STM32F103C8T6 链接脚本（GNU ld）
 *
 * @memory 64 KB Flash @ 0x0800_0000，20 KB SRAM @ 0x2000_0000
 * @stack  栈顶 _estack = RAM 末尾（满递减栈）
 *
 * GNU ld 顶层命令（自上而下）：
 *   ENTRY     — 程序入口符号（Reset_Handler）
 *   MEMORY    — 定义物理存储区域（Flash/RAM 基址、容量、访问属性）
 *   SECTIONS  — 将各 .o 的输入段合并为输出段，并映射到 MEMORY 区域
 *
 * @note    符号 _sidata/_sdata/_edata/_sbss/_ebss 供 startup 初始化段使用
 * @note    ORIGIN 为 Main Flash 物理基址（RM0008）；复位时 CPU 经逻辑 0x00000000
 *          别名访问同内容（BOOT0=0），详见 doc/learn/stm32f103-memory-boot-map.md
 * @see     doc/learn/f103-manual-build-from-scratch.md §2.1 — MEMORY / SECTIONS 语义
 */

ENTRY(Reset_Handler)

/* Cortex-M3 主栈顶：SRAM 最高地址（栈向下增长） */
_estack = 0x20005000;

/* MEMORY：定义芯片上的物理存储区域（链接器分配时的「容器」） */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 20K
}

/*
 * SECTIONS：GNU ld 段布局命令（非 C 关键字）。
 * 规定如何将各目标文件的输入段（如 .text、.data）合并为输出段，
 * 以及输出段的 VMA（运行地址）与 LMA（镜像/load 地址，.data 等需要）。
 * > FLASH / > RAM 表示 VMA；.data 的 "AT > FLASH" 表示 LMA 在 Flash。
 */
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
