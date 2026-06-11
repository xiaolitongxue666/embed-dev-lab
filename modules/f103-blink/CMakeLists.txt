# -----------------------------------------------------------------------------
# f103-blink — STM32F103C8T6 PC13 LED 闪烁模块
# 构建: 仓库根目录 ./scripts/build.sh f103-blink
# 可独立作为工程根打开，也可由顶层 CMakeLists.txt add_subdirectory 引入
# -----------------------------------------------------------------------------

# 单独打开本模块目录时，自行声明 project
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.20)
    project(f103-blink C ASM)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/mcu-config.cmake)

set(F103_SOURCES
    src/main.c                  # 应用入口与 GPIO 闪烁
    src/system_stm32f10x.c      # SystemInit / 72 MHz 时钟
    startup/startup_stm32f103xb.s # 向量表、.data/.bss、跳转 main
)

# 生成 f103-blink.elf（链接）+ f103-blink.hex（POST_BUILD objcopy，见 mcu-config.cmake）
embed_mcu_add_executable(f103-blink
    SOURCES ${F103_SOURCES}
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
    LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker/stm32f103c8.ld
    MCU_FLAGS "-mcpu=cortex-m3 -mthumb"
)
