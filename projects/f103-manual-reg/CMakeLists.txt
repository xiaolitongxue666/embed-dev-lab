# -----------------------------------------------------------------------------
# f103-manual-reg — STM32F103C8T6 PC13 + USART1 + ADC1 PA0 + SPI1 BMP280/LSM6 + I2C1 SH1106（全手写寄存器，无 CMSIS/HAL）
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
    src/main.c                  # 应用入口：LED、BMP280 温度、WHO_AM_I、六轴轮询、串口回显
    src/nvic.c                  # NVIC ISER/IP 手写（USART1 / DMA1 CH4/CH5/CH6）
    src/dma.c                   # DMA1 通道启停（USART1 TX=CH4 RX=CH5；I2C1 TX=CH6）
    src/adc.c                   # ADC1 CH0（PA0）单次转换
    src/usart.c                 # USART1 DMA 收发 + 空闲定界
    src/spi.c                   # SPI1 双 CS：PA3=BMP280 Mode0，PA8=LSM6 Mode3
    src/bmp280.c                # BMP280 / BME280 SPI 校准与补偿温度/气压
    src/lsm6ds3.c               # LSM6DS3 SPI 读写与 raw 采样（CS=PA8）
    src/i2c.c                   # I2C1 PB6/PB7；命令轮询，页数据 DMA1 CH6
    src/sh1106.c                # SH1106 帧缓冲：中央时钟 + 右下温度
    src/sh1106_font.c           # 8x16 数字/冒号/小数点/负号/C
    src/systick.c               # SysTick 1 ms
    src/syscalls.c              # printf → USART1
    src/system_stm32f1xx.c      # SystemInit / 72 MHz 时钟
    startup/startup_stm32f103xb.s # 向量表（含外设 IRQ）、.data/.bss、跳转 main
)

# 生成 f103-manual-reg.elf（链接）+ f103-manual-reg.hex（POST_BUILD objcopy，见 mcu-config.cmake）
embed_mcu_add_executable(f103-manual-reg
    SOURCES ${F103_SOURCES}
    INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
    LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32F103C8_FLASH.ld
    MCU_FLAGS "-mcpu=cortex-m3 -mthumb"
)
