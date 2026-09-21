# -----------------------------------------------------------------------------
# f103-manual-reg — STM32F103C8T6 PC13 + USART1 + ADC1 PA0 + SPI1 BMP280 + TIM2 JY003 + I2C1 SH1106（全手写寄存器，无 CMSIS/HAL）
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
    src/app/main.c
    src/app/log.c
    src/board/gpio.c
    src/board/key.c
    src/periph/nvic.c
    src/periph/dma.c
    src/periph/adc.c
    src/periph/usart.c
    src/periph/spi.c
    src/periph/tim2.c
    src/periph/i2c.c
    src/periph/systick.c
    src/periph/timer_event.c
    src/periph/syscalls.c
    src/periph/system_stm32f1xx.c
    src/driver/bmp280.c
    src/driver/lsm6ds3.c
    src/driver/sh1106.c
    src/driver/sh1106_font.c
    startup/startup_stm32f103xb.s
)

# 生成 f103-manual-reg.elf（链接）+ f103-manual-reg.hex（POST_BUILD objcopy，见 mcu-config.cmake）
embed_mcu_add_executable(f103-manual-reg
    SOURCES ${F103_SOURCES}
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/app
        ${CMAKE_CURRENT_SOURCE_DIR}/src/board
        ${CMAKE_CURRENT_SOURCE_DIR}/src/periph
        ${CMAKE_CURRENT_SOURCE_DIR}/src/driver
    LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32F103C8_FLASH.ld
    MCU_FLAGS "-mcpu=cortex-m3 -mthumb"
)
