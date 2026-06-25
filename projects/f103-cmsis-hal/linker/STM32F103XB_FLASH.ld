/*
*****************************************************************************
**
**  File        : STM32F103XB_FLASH.ld
**
**  Abstract    : Linker script for STM32F103xB Device with
**                128KByte FLASH, 20KByte RAM
**
**                Set heap size, stack size and stack location according
**                to application requirements.
**
**                Set memory bank area and size if external memory is used.
**
**  Target      : STMicroelectronics STM32
**
**
**  Distribution: The file is distributed as is, without any warranty
**                of any kind.
**
**  (c)Copyright Ac6.
**  You may use this file as-is or modify it according to the needs of your
**  project. Distribution of this file (unmodified or modified) is not
**  permitted. Ac6 permit registered System Workbench for MCU users the
**  rights to distribute the assembled, compiled & linked contents of this
**  file as part of an application binary file, provided that it is built
**  using the System Workbench for MCU toolchain.
**
*****************************************************************************
**
**  @file    STM32F103XB_FLASH.ld（embed-dev-lab 维护注释）
**  @brief   STM32F103C8T6 GNU ld 链接脚本（源自 CMSIS xB 模板，C8 容量裁剪）
**
**  @memory 64 KB Flash @ 0x0800_0000，20 KB SRAM @ 0x2000_0000
**  @stack  栈顶 _estack = RAM 末尾 0x20005000（满递减栈；push 时 SP 向低地址减小）
**
**  GNU ld 顶层命令（自上而下）：
**    ENTRY     — 程序入口符号 Reset_Handler（与向量表 +4 项一致）
**    MEMORY    — 定义 Flash/RAM 物理区域（ORIGIN、LENGTH、访问属性）
**    SECTIONS  — 将各 .o 的输入段合并为输出段，并映射到 MEMORY
**
**  与 startup 的契约（符号须一致，否则 Reset 后 RAM 未正确初始化）：
**    _sidata  — .data 初值在 Flash 中的 LMA 起始（LOADADDR）
**    _sdata   — .data 在 RAM 中的 VMA 起始
**    _edata   — .data 在 RAM 中的 VMA 结束（不含）
**    _sbss    — .bss  在 RAM 中的 VMA 起始
**    _ebss    — .bss  在 RAM 中的 VMA 结束（不含）
**    _estack  — 初始 MSP；写入向量表第 0 项，硬件复位时自动加载
**
**  @note    本文件由 fetch 自 cmsis-device-f1 拷贝后经 C8 裁剪（64K Flash、_estack）；
**           fetch 后会自动 apply 中文注释补丁，手工改动注释后重跑 fetch 可恢复。
**  @see     doc/learn/linker-vma-lma.md — VMA/LMA 说明
**  @see     doc/learn/stm32f103-memory-boot-map.md — Flash 启动与 0x08000000 别名
**
*****************************************************************************
*/

/* 程序入口：链接器记录 ELF 入口点；Cortex-M3 实际从向量表 +4 跳入 Reset_Handler */
ENTRY(Reset_Handler)

/* Cortex-M3 主栈顶 = SRAM 上界；C8T6：0x20000000 + 20K = 0x20005000 */
_estack = 0x20005000;

/* 链接时预留最小堆/栈空间；._user_heap_stack 段在 RAM 末尾占位，防止 .bss 侵占栈区 */
_Min_Heap_Size  = 0x200;   /* 512 B 堆 */
_Min_Stack_Size = 0x400;   /* 1 KB 栈 */

/*
 * MEMORY：地址池。SECTIONS 中 "> FLASH" / "> RAM" 指定各段 VMA；
 * .data 另用 "AT > FLASH" 指定 LMA（烧录镜像中的位置）。
 */
MEMORY
{
  FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 64K   /* 主 Flash；rx = 可读可执行 */
  RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 20K   /* SRAM；xrw = 可读可写可执行（本 demo 代码不在 RAM 跑） */
}

/*
 * SECTIONS：段布局。顺序影响 Flash/RAM 中各段地址。
 * "." 为位置计数器；ALIGN(4) 保证 4 字节对齐（Cortex-M 字访问要求）。
 */
SECTIONS
{
  /* ---------- .isr_vector：中断向量表，必须位于 Flash 物理起始 0x08000000 ---------- */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))   /* 收集 startup 中 g_pfnVectors；KEEP 防止 --gc-sections 丢弃 */
    . = ALIGN(4);
  } > FLASH                 /* VMA = LMA：向量表仅存在于 Flash，复位时硬件从 0x08000000 读取 */

  /* ---------- .text：机器码；VMA/LMA 均在 Flash（XIP 原地取指） ---------- */
  .text :
  {
    . = ALIGN(4);
    *(.text)                 /* 函数机器码（main、HAL、SystemInit 等） */
    *(.text*)                /* .text.* 子段（如 .text.Reset_Handler） */
    *(.glue_7)               /* ARM/Thumb 状态切换胶合代码 */
    *(.glue_7t)
    *(.eh_frame)             /* 异常帧（通常嵌入式可忽略） */

    KEEP (*(.init))          /* C 库 init 段（nosys 裸机通常为空） */
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;              /* 符号：Flash 中代码段结束地址（rodata 之前） */
  } > FLASH

  /* ---------- .rodata：只读常量（字符串字面量、const 全局变量） ---------- */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } > FLASH

  /* ARM 异常索引表等；C 裸机 demo 通常无 C++ 异常，段可能为空 */
  .ARM.extab   : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
  .ARM : {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } > FLASH

  /* C/C++ 全局构造/析构函数表；本 demo startup 已跳过 __libc_init_array，段通常为空 */
  .preinit_array     :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } > FLASH
  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } > FLASH
  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } > FLASH

  /* .data 的 LMA 起点：startup 拷贝源 = Flash 中已初始化变量的镜像起始地址 */
  _sidata = LOADADDR(.data);

  /* ---------- .data：已初始化全局/静态变量；运行时 VMA 在 RAM，初值 LMA 在 Flash ---------- */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;              /* startup：拷贝目标起点（RAM） */
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;              /* startup：拷贝目标终点（不含） */
  } > RAM AT> FLASH          /* VMA>RAM 运行时读写；AT>FLASH 烧录镜像中的位置 */

  /* ---------- .bss：未初始化全局/静态变量；仅占 RAM，Flash 镜像无对应内容，上电须清零 ---------- */
  . = ALIGN(4);
  .bss :
  {
    _sbss = .;               /* startup：清零循环起点 */
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)                /* 旧式 COMMON 块（gcc 可能生成） */
    . = ALIGN(4);
    _ebss = .;               /* startup：清零循环终点（不含） */
    __bss_end__ = _ebss;
  } > RAM

  /* 在 RAM 末尾预留堆+栈空间；链接阶段检查是否溢出（.bss/.data 不可侵占此区域） */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } > RAM

  /* 丢弃静态库中的冗余段符号，减小 ELF 体积 */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
