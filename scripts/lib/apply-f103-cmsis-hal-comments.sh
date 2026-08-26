#!/usr/bin/env bash
# -----------------------------------------------------------------------------
# f103-cmsis-hal：CMSIS 模板拷贝后恢复中文注释（仅改注释，不改代码）
# 由 fetch-f103-cmsis-hal-deps.sh 在 copy_templates 之后调用
# -----------------------------------------------------------------------------

apply_f103_cmsis_hal_comments() {
  local root="$1"
  local proj="$root/projects/f103-cmsis-hal"
  local startup="$proj/startup/startup_stm32f103xb.s"
  local linker="$proj/linker/STM32F103XB_FLASH.ld"
  local system="$proj/src/system_stm32f1xx.c"

  [[ -f "$startup" ]] || return 0

  # startup：用 embed-dev-lab 带详细中文注释的模板覆盖 CMSIS 裸拷贝（fetch 后恢复）
  local startup_tpl="$root/scripts/lib/templates/f103-cmsis-hal/startup_stm32f103xb.s"
  if [[ -f "$startup_tpl" ]]; then
    cp -f "$startup_tpl" "$startup"
  else
  # 无模板时回退：对 CMSIS 原文做 sed 注释补丁
  sed -i \
    -e 's|/\* start address for the initialization values of the .data section\.\ndefined in linker script \*/|/* .data 初值在 Flash 中的加载地址（LMA），由链接脚本定义 */|' \
    -e 's|/\* start address for the initialization values of the .data section\. defined in linker script \*/|/* .data 初值在 Flash 中的加载地址（LMA），由链接脚本定义 */|' \
    "$startup"

  sed -i \
    -e 's|/\* start address for the .data section\. defined in linker script \*/|/* .data 在 RAM 中的起始地址（VMA） */|' \
    -e 's|/\* end address for the .data section\. defined in linker script \*/|/* .data 在 RAM 中的结束地址（不含） */|' \
    -e 's|/\* start address for the .bss section\. defined in linker script \*/|/* .bss 在 RAM 中的起始地址 */|' \
    -e 's|/\* end address for the .bss section\. defined in linker script \*/|/* .bss 在 RAM 中的结束地址（不含） */|' \
    "$startup"

  sed -i \
    -e 's|/\* Call the clock system initialization function\.\*/|/* 调用 SystemInit（复位默认化；本 demo 时钟在 main 中 HAL 配置） */|' \
    -e 's|/\* Copy the data segment initializers from flash to SRAM \*/|/* 将 .data 从 Flash 拷贝到 SRAM */|' \
    -e 's|/\* Zero fill the bss segment\. \*/|/* 将 .bss 段清零 */|' \
    -e 's|/\* Call the application'\''s entry point\.\*/|/* 进入应用 main */|' \
    "$startup"

  # startup 文件头 @note（若尚未存在）
  if ! grep -q 'embed-dev-lab：Reset 流程' "$startup"; then
    sed -i '/^\s*\*\s*All rights reserved\./a\
  * @note    embed-dev-lab：Reset 流程 SystemInit → 拷贝 .data → 清零 .bss → main；\
  *          已跳过 __libc_init_array（nosys 裸机，避免 _init 链接错误）。' "$startup"
  fi

  # Reset_Handler 中文 brief（若仍为英文模板）
  if grep -q 'This is the code that gets called' "$startup"; then
    sed -i '/\.equ  BootRAM,/a\
/**\
 * @brief  复位后入口：SystemInit → .data 搬运 → .bss 清零 → main\
 */' "$startup"
    sed -i '/This is the code that gets called/,+2d' "$startup"
  fi

  # nosys：跳过 C++ 全局构造（fetch 已删 bl，补注释占位）
  if ! grep -q '__libc_init_array' "$startup"; then
    sed -i '/\/\* 进入应用 main \*\//i\
/* 裸机 nosys 无 C++ 全局构造，跳过 __libc_init_array */\
/* bl __libc_init_array */' "$startup" 2>/dev/null || \
    sed -i '/\/\* Call the application'\''s entry point\.\*\//i\
/* 裸机 nosys 无 C++ 全局构造，跳过 __libc_init_array */\
/* bl __libc_init_array */' "$startup"
  fi
  fi

  # linker 行尾注释 + embed-dev-lab @note
  if [[ -f "$linker" ]]; then
    if ! grep -q 'embed-dev-lab：STM32F103C8T6' "$linker"; then
      sed -i '/^\*\*\  Distribution: The file is distributed as is/i\
**  @note embed-dev-lab：STM32F103C8T6，Flash 64K / RAM 20K；\
**        _estack=0x20005000；符号 _sidata/_sdata/_sbss/_ebss 供 startup 使用。\
**        源自 CMSIS STM32F103XB_FLASH.ld（C8 容量裁剪）。\
**' "$linker"
    fi
    sed -i \
      -e 's|128KByte FLASH, 20KByte RAM|64KByte FLASH, 20KByte RAM (embed-dev-lab C8; CMSIS template was 128K)|' \
      -e 's|/\* Entry Point \*/|/* 程序入口符号 */|' \
      -e 's|/\* Highest address of the user mode stack \*/|/* 主栈顶（满递减栈上界） */|' \
      -e 's|/\* end of RAM \*/|/* RAM 末尾；C8T6：0x20005000 */|' \
      -e 's|/\* required amount of heap  \*/|/* 最小堆保留 */|' \
      -e 's|/\* required amount of stack \*/|/* 最小栈保留 */|' \
      -e 's|/\* Specify the memory areas \*/|/* 存储区域：C8T6 Flash 64K / RAM 20K */|' \
      -e 's|/\* Define output sections \*/|/* 输出段布局；符号供 startup 初始化 .data/.bss */|' \
      -e 's|/\* The startup code goes first into FLASH \*/|/* 向量表须固定 Flash 起始 */|' \
      -e 's|KEEP(\*(.isr_vector)) /\* Startup code \*/|KEEP(*(.isr_vector)) /* 中断向量表 */|' \
      -e 's|/\* The program code and other data goes into FLASH \*/|/* 代码与只读数据 */|' \
      -e 's|/\* Constant data goes into FLASH \*/|/* 只读常量 */|' \
      -e 's|/\* used by the startup to initialize data \*/|/* startup 用：.data 的 LMA */|' \
      -e 's|/\* Initialized data sections goes into RAM, load LMA copy after code \*/|/* .data：VMA 在 RAM，LMA 在 Flash */|' \
      -e 's|/\* Uninitialized data section \*/|/* .bss：仅 VMA，上电须清零 */|' \
      -e 's|/\* This is used by the startup in order to initialize the .bss section \*/|/* startup 清零 .bss 用 */|' \
      -e 's|/\* User_heap_stack section, used to check that there is enough RAM left \*/|/* 堆栈预留；链接时检查 RAM 是否足够 */|' \
      -e 's|/\* Remove information from the standard libraries \*/|/* 丢弃标准库冗余段 */|' \
      "$linker"
  fi

  # system_stm32f1xx.c：中文头 + 关键 inline 注释
  if [[ -f "$system" ]]; then
    if ! grep -q 'embed-dev-lab 中文说明' "$system"; then
      sed -i '1i\
/**\
 * @file    system_stm32f1xx.c\
 * @brief   CMSIS SystemInit 模板（复位后由 startup 调用）\
 * @target  STM32F103C8T6\
 * @note    embed-dev-lab 中文说明：本 demo 时钟在 main.c 的 SystemClock_Config() 中由 HAL 配置；\
 *          SystemInit 仅做向量表等复位默认化。下方 ST 原文与 Copyright 保留。\
 */\
' "$system"
    fi
    sed -i \
      -e 's|  /\* This variable is updated in three ways:|  /* 本 demo 由 HAL 更新 SystemCoreClock；也可调用 SystemCoreClockUpdate() 或 HAL_RCC_GetHCLKFreq() */\
  /* 更新方式：|' \
      -e 's|      1) by calling CMSIS function SystemCoreClockUpdate()|      1) 调用 CMSIS SystemCoreClockUpdate()|' \
      -e 's|      2) by calling HAL API function HAL_RCC_GetHCLKFreq()|      2) 调用 HAL API HAL_RCC_GetHCLKFreq()|' \
      -e 's|      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency|      3) 每次 HAL_RCC_ClockConfig() 后自动更新（本 demo 采用）|' \
      -e '/Note: If you use this function to configure the system clock/,/variable is updated automatically\./d' \
      -e 's|/\* Note: Following vector table addresses must be defined in line with linker|/* 向量表地址须与链接脚本一致（本 demo 未启用 USER_VECT_TAB_ADDRESS）|' \
      -e 's|         configuration\. \*/| */|' \
      -e 's|  /\* Configure the Vector Table location ------------------------------------- \*/|  /* 向量表重定位（本 demo 未启用 USER_VECT_TAB_ADDRESS） */|' \
      -e 's|/\* Vector Table Relocation in Internal SRAM\. \*/|/* 向量表重定位 */|' \
      -e 's|  \* @brief  Setup the microcontroller system|  * @brief  复位后系统默认化（本 demo 不在此配置 PLL 时钟）|' \
      -e 's|  \*         Initialize the Embedded Flash Interface, the PLL and update the|  * @note   72 MHz 时钟见 main.c SystemClock_Config()|' \
      -e '/Initialize the Embedded Flash Interface/d' \
      -e '/SystemCoreClock variable\./d' \
      -e '/This function should be used only after reset\./d' \
      -e 's|  \* @brief  Update SystemCoreClock variable according to Clock Register Values\.|  * @brief  根据 RCC 寄存器更新 SystemCoreClock（HCLK）|' \
      -e '/The SystemCoreClock variable contains the core clock/,/based on this variable will be incorrect\./d' \
      -e '/- The system frequency computed by this function/,/value for HSE crystal\./d' \
      -e 's|  \* @note   - The system frequency computed|  * @note   本 demo 主要依赖 HAL_RCC_ClockConfig 自动维护；读寄存器推算频率见 ST 原文注释|' \
      -e 's|  \* Get SYSCLK source ------------------------------------------------------- \*/|  /* 读取 SYSCLK 来源 -------------------------------------------------------*/|' \
      -e 's|    case 0x00U:  /\* HSI used as system clock \*/|    case 0x00U:  /* HSI 作为系统时钟 */|' \
      -e 's|    case 0x04U:  /\* HSE used as system clock \*/|    case 0x04U:  /* HSE 作为系统时钟 */|' \
      -e 's|    case 0x08U:  /\* PLL used as system clock \*/|    case 0x08U:  /* PLL 作为系统时钟 */|' \
      -e 's|      /\* Get PLL clock source and multiplication factor ---------------------- \*/|      /* 读取 PLL 时钟源与倍频系数 */|' \
      -e 's|        /\* HSI oscillator clock divided by 2 selected as PLL clock entry \*/|        /* HSI/2 作为 PLL 输入 */|' \
      -e 's|       /\* HSE oscillator clock selected as PREDIV1 clock entry \*/|       /* HSE 作为 PREDIV1 输入 */|' \
      -e 's|        /\* HSE selected as PLL clock entry \*/|        /* HSE 作为 PLL 输入 */|' \
      -e 's|        {/\* HSE oscillator clock divided by 2 \*/|        {/* HSE 二分频 */|' \
      -e 's|      {/\* PREDIV1 selected as PLL clock entry \*/|      {/* PREDIV1 作为 PLL 输入 */|' \
      -e 's|        /\* Get PREDIV1 clock source and division factor \*/|        /* 读取 PREDIV1 时钟源与分频 */|' \
      -e 's|          /\* HSE oscillator clock selected as PREDIV1 clock entry \*/|          /* HSE 作为 PREDIV1 输入 */|' \
      -e 's|        {/\* PLL2 clock selected as PREDIV1 clock entry \*/|        {/* PLL2 作为 PREDIV1 输入 */|' \
      -e 's|          /\* Get PREDIV2 division factor and PLL2 multiplication factor \*/|          /* 读取 PREDIV2 分频与 PLL2 倍频 */|' \
      -e 's|      { /\* PLL multiplication factor = PLL input clock \* 6\.5 \*/|      { /* PLL 倍频系数 = 输入时钟 × 6.5 */|' \
      -e 's|  /\* Compute HCLK clock frequency ---------------- \*/|  /* 计算 HCLK 频率 */|' \
      -e 's|  /\* Get HCLK prescaler \*/|  /* 读取 AHB 预分频 */|' \
      -e 's|  /\* HCLK clock frequency \*/|  /* HCLK = SYSCLK / 预分频 */|' \
      "$system" 2>/dev/null || true
  fi

  log_ok "Applied f103-cmsis-hal Chinese comment patches"
  apply_f103_third_party_embed_notes "$root"
}

# third_party：在 ST 文件头前插入 embed-dev-lab 中文说明（不翻译 vendor 正文）
inject_third_party_embed_note() {
  local file="$1"
  local note="$2"
  [[ -f "$file" ]] || return 0
  grep -q 'embed-dev-lab third_party' "$file" && return 0
  local block
  block="/**
 * @note    embed-dev-lab third_party：${note}
 *          ST 下方原文与 Copyright 保留；fetch 后由 apply 脚本恢复本注释块。
 */

"
  { printf '%s' "$block"; cat "$file"; } > "${file}.embed.tmp" && mv "${file}.embed.tmp" "$file"
}

apply_f103_third_party_embed_notes() {
  local root="$1"
  local tp="$root/projects/f103-cmsis-hal/third_party"
  local cmsis="$tp/cmsis/Include"
  local hal_inc="$tp/hal/Inc"
  local hal_src="$tp/hal/Src"

  inject_third_party_embed_note "$cmsis/core_cm3.h" \
    "Cortex-M3 内核（NVIC/SysTick）；startup 与 HAL 共用"
  inject_third_party_embed_note "$cmsis/cmsis_compiler.h" \
    "CMSIS 编译器抽象入口"
  inject_third_party_embed_note "$cmsis/cmsis_version.h" \
    "CMSIS 版本标识"
  inject_third_party_embed_note "$cmsis/cmsis_gcc.h" \
    "GCC/Arm Compiler intrinsic 与 barrier"
  inject_third_party_embed_note "$cmsis/stm32f1xx.h" \
    "F1 系列设备入口；CMake 定义 USE_HAL_DRIVER、STM32F103xB"
  inject_third_party_embed_note "$cmsis/stm32f103xb.h" \
    "F103xB 寄存器映射（RCC/GPIO/PWR 等）；HAL 底层读写"
  inject_third_party_embed_note "$cmsis/system_stm32f1xx.h" \
    "SystemInit/SystemCoreClock 声明；实现在 src/system_stm32f1xx.c"

  inject_third_party_embed_note "$hal_inc/stm32f1xx_hal.h" \
    "HAL 主头；由 src/stm32f1xx_hal_conf.h 裁剪包含模块"

  inject_third_party_embed_note "$hal_src/stm32f1xx_hal.c" \
    "HAL_Init、SysTick、HAL_IncTick；main 与 stm32f1xx_it.c 使用"
  inject_third_party_embed_note "$hal_src/stm32f1xx_hal_cortex.c" \
    "NVIC/SysTick 配置；HAL_InitTick 内部调用"
  inject_third_party_embed_note "$hal_src/stm32f1xx_hal_gpio.c" \
    "GPIO 初始化与写引脚；main.c MX_GPIO_Init / WritePin"
  inject_third_party_embed_note "$hal_src/stm32f1xx_hal_rcc.c" \
    "RCC 时钟配置；main.c SystemClock_Config"
  inject_third_party_embed_note "$hal_src/stm32f1xx_hal_rcc_ex.c" \
    "F103 PLL 扩展；配合 HAL_RCC_OscConfig"
  inject_third_party_embed_note "$hal_src/stm32f1xx_hal_pwr.c" \
    "电源与 Backup 域；main.c HAL_PWR_EnableBkUpAccess（PC13 前置）"
  inject_third_party_embed_note "$hal_src/stm32f1xx_hal_flash.c" \
    "Flash 等待周期；ClockConfig 时 FLASH_LATENCY_2"
  inject_third_party_embed_note "$hal_src/stm32f1xx_hal_flash_ex.c" \
    "Flash 扩展操作；RCC 配置依赖"
  inject_third_party_embed_note "$hal_src/stm32f1xx_hal_uart.c" \
    "USART 阻塞发送；usart.c USART1_WriteStr → HAL_UART_Transmit"

  log_ok "Applied f103-cmsis-hal third_party embed notes"
}
