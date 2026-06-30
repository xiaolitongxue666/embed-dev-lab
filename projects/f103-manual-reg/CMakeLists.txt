# -----------------------------------------------------------------------------
# f103-manual-reg — STM32F103C8T6 PC13 闪烁 + USART1 printf（全手写寄存器，无 CMSIS/HAL）
# 构建: 仓库根目录 ./scripts/build.sh f103-manual-reg
# 一键:  ./scripts/build-flash.sh
# 文档:  doc/projects/f103-manual-reg.md
# -----------------------------------------------------------------------------

# 单独打开本工程目录时，自行声明 project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.20)
    project(f103-manual-reg C ASM)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/mcu-config.cmake)

set(F103_SOURCES
    src/main.c                  # 应用入口与 GPIO 闪烁
    src/usart.c                 # USART1 纯寄存器初始化
    src/syscalls.c              # printf → USART1
    src/system_stm32f1xx.c      # SystemInit / 72 MHz 时钟
    startup/startup_stm32f103xb.s # 向量表、.data/.bss、跳转 main
)

# 生成 f103-manual-reg.elf（链接）+ f103-manual-reg.hex（POST_BUILD objcopy，见 mcu-config.cmake）
embed_mcu_add_executable(f103-manual-reg
    SOURCES ${F103_SOURCES}
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
    LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32F103C8_FLASH.ld
    MCU_FLAGS "-mcpu=cortex-m3 -mthumb"
)
