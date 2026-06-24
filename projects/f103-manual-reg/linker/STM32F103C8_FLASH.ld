/**
 * @file    STM32F103C8_FLASH.ld
 * @brief   STM32F103C8T6 链接脚本（GNU ld）
 *
 * @memory 64 KB Flash @ 0x0800_0000，20 KB SRAM @ 0x2000_0000
 * @stack  栈顶 _estack = RAM 末尾（满递减栈）；push 时 SP 向低地址减小
 *
 * GNU ld 顶层命令（自上而下）：
 *   ENTRY     — 程序入口符号（Reset_Handler）
 *   MEMORY    — 定义物理存储区域（Flash/RAM 基址、容量、访问属性）
 *   SECTIONS  — 将各 .o 的输入段合并为输出段，并映射到 MEMORY 区域
 *
 * @note    符号 _sidata/_sdata/_edata/_sbss/_ebss 供 startup 初始化段使用
 * @note    ORIGIN 为 Main Flash 物理基址（RM0008）；复位时 CPU 经逻辑 0x00000000
 *          别名访问同内容（BOOT0=0），详见 doc/learn/stm32f103-memory-boot-map.md
 * @see     doc/learn/linker-vma-lma.md — VMA/LMA 权威说明（逐段对照本文件）
 * @see     doc/learn/f103-manual-build-from-scratch.md §2.1 — MEMORY / SECTIONS / ALIGN / KEEP
 */

ENTRY(Reset_Handler)

/* Cortex-M3 主栈顶：SRAM 上界 0x20005000（= ORIGIN+LENGTH）；满递减栈，push 时 SP 减小 */
_estack = 0x20005000;

/* MEMORY：Flash/RAM 地址池；SECTIONS 中 > 区域 指定各段 VMA，AT> 可单独指定 LMA */
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
    /* ---------- 输出段 .isr_vector：中断向量表，须固定 Flash 物理起始 0x08000000 ---------- */
    .isr_vector :                        /* 独立输出段；排在 .text 之前，故链接后位于 Flash 最前 */
    {
        . = ALIGN(4);                    /* . 为位置计数器：向上对齐到 4 字节（向量表每项 4 字节） */
        KEEP(*(.isr_vector))             /* 从所有 .o 收集 .isr_vector；KEEP 防止 --gc-sections 丢弃向量表 */
        . = ALIGN(4);                    /* 段尾 4 字节对齐 */
    } > FLASH                            /* VMA=LMA：均在 Flash；复位从 0x08000000 读 MSP/Reset */

    /* ---------- 输出段 .text：机器码与只读常量，VMA/LMA 均在 Flash（XIP 执行） ---------- */
    .text :                              /* 定义名为 .text 的输出段（ELF 中的代码段） */
    {
        . = ALIGN(4);                    /* 位置计数器 . 对齐到 4 字节边界（Cortex-M 指令/字访问要求） */
        *(.text)                         /* 从所有 .o 收集输入段 .text（函数机器码，如 main、SystemInit） */
        *(.text*)                        /* 收集 .text.* 子段（如 .text.Reset_Handler、.text.Default_Handler） */
        *(.rodata)                       /* 收集 .rodata：只读数据（字符串字面量、const 全局变量） */
        *(.rodata*)                      /* 收集 .rodata.* 子段（编译器可能拆分的只读数据） */
        . = ALIGN(4);                    /* 段尾再次 4 字节对齐，便于下一段或 LMA 计算对齐 */
        _etext = .;                      /* 符号：.text 段末尾地址（Flash 中代码+rodata 结束位置） */
    } > FLASH                            /* VMA=LMA：均在 Flash；XIP 原地取指，无需搬运 */

    /* .data 的 LMA 起点（Flash 镜像）；VMA 见下方 _sdata/_edata（RAM） */
    _sidata = LOADADDR(.data);           /* LOADADDR：取出 .data 烧录地址（LMA），供 startup 作拷贝源 */

    /* ---------- 输出段 .data：已初始化全局/静态变量，运行时 VMA 在 RAM，初值 LMA 在 Flash ---------- */
    .data :                              /* 定义 .data 输出段（有非零初值的全局变量） */
    {
        . = ALIGN(4);                    /* RAM 中 .data 起始地址 4 字节对齐 */
        _sdata = .;                      /* 符号：.data 在 RAM 的起始 VMA；startup 拷贝目标起点 */
        *(.data)                         /* 从所有 .o 收集 .data 输入段（已初始化全局/静态变量） */
        *(.data*)                        /* 收集 .data.* 子段（编译器拆分的数据） */
        . = ALIGN(4);                    /* .data 段尾对齐 */
        _edata = .;                      /* 符号：.data 在 RAM 的结束地址（不含）；startup 拷贝终点 */
    } > RAM AT > FLASH                   /* VMA>RAM 运行时读写；AT>FLASH 指定 LMA，初值随固件烧录进 Flash */

    /* ---------- 输出段 .bss：未初始化全局/静态变量，仅占 RAM，Flash 镜像中无对应内容 ---------- */
    .bss :                               /* 定义 .bss 输出段（C 中未显式赋初值的全局变量，初值为 0） */
    {
        . = ALIGN(4);                    /* .bss 在 RAM 的起始地址 4 字节对齐（紧接 .data 之后） */
        _sbss = .;                       /* 符号：.bss 起始地址；startup 清零循环起点 */
        *(.bss)                          /* 从所有 .o 收集 .bss 输入段 */
        *(.bss*)                         /* 收集 .bss.* 子段 */
        *(COMMON)                        /* 收集 COMMON 块（旧式未初始化全局符号，gcc 可能生成） */
        . = ALIGN(4);                    /* .bss 段尾对齐 */
        _ebss = .;                       /* 符号：.bss 结束地址（不含）；startup 清零循环终点 */
    } > RAM                              /* 仅 VMA>RAM；无 LMA/AT，不占 Flash，startup 须清零 _sbss.._ebss */
}
