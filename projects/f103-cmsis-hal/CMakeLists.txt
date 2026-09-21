# -----------------------------------------------------------------------------
# f103-cmsis-hal — STM32F103C8T6 CMSIS + HAL 对照 demo
# 构建框架与 f103-manual-reg 一致；链接脚本为 CMSIS STM32F103XB_FLASH.ld
# 构建: ./scripts/build.sh f103-cmsis-hal
# 依赖: ./scripts/fetch-f103-cmsis-hal-deps.sh
# -----------------------------------------------------------------------------

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.20)
    project(f103-cmsis-hal C ASM)
endif()

include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/mcu-config.cmake)

set(CMSIS_INC ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cmsis/Include)
set(HAL_INC   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/hal/Inc)
set(HAL_SRC   ${CMAKE_CURRENT_SOURCE_DIR}/third_party/hal/Src)

set(F103_HAL_SOURCES
    src/main.c
    src/gpio.c
    src/usart.c
    src/spi.c
    src/adc.c
    src/tim.c
    src/i2c.c
    src/key.c
    src/driver/bmp280.c
    src/driver/lsm6ds3.c
    src/driver/sh1106.c
    src/driver/sh1106_font.c
    src/system_stm32f1xx.c
    src/stm32f1xx_hal_msp.c
    src/stm32f1xx_it.c
    startup/startup_stm32f103xb.s
    ${HAL_SRC}/stm32f1xx_hal.c
    ${HAL_SRC}/stm32f1xx_hal_cortex.c
    ${HAL_SRC}/stm32f1xx_hal_gpio.c
    ${HAL_SRC}/stm32f1xx_hal_rcc.c
    ${HAL_SRC}/stm32f1xx_hal_rcc_ex.c
    ${HAL_SRC}/stm32f1xx_hal_pwr.c
    ${HAL_SRC}/stm32f1xx_hal_flash.c
    ${HAL_SRC}/stm32f1xx_hal_flash_ex.c
    ${HAL_SRC}/stm32f1xx_hal_uart.c
    ${HAL_SRC}/stm32f1xx_hal_dma.c
    ${HAL_SRC}/stm32f1xx_hal_spi.c
    ${HAL_SRC}/stm32f1xx_hal_i2c.c
    ${HAL_SRC}/stm32f1xx_hal_adc.c
    ${HAL_SRC}/stm32f1xx_hal_adc_ex.c
    ${HAL_SRC}/stm32f1xx_hal_tim.c
    ${HAL_SRC}/stm32f1xx_hal_tim_ex.c
)

add_compile_definitions(
    STM32F103xB
    USE_HAL_DRIVER
    HSE_VALUE=8000000U
)

embed_mcu_add_executable(f103-cmsis-hal
    SOURCES ${F103_HAL_SOURCES}
    INCLUDE_DIRS
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/driver
        ${CMSIS_INC}
        ${HAL_INC}
    LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32F103XB_FLASH.ld
    MCU_FLAGS "-mcpu=cortex-m3 -mthumb"
)
