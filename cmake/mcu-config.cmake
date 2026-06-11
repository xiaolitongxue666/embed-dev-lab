# -----------------------------------------------------------------------------
# 裸机 MCU 模块公共构建函数
# 各 modules/<name>/CMakeLists.txt 通过 embed_mcu_add_executable 声明固件目标
#
# 构建产物（以 f103-blink 为例）:
#   build 后 Ninja 链接 → <name>.elf
#   POST_BUILD 自动     → <name>.hex（arm-none-eabi-objcopy -O ihex）
#   当前不生成 .bin；烧录主路径 probe-rs 直接使用 .elf
#
# 工具链说明:
#   本项目使用 arm-none-eabi-gcc（裸机），非 arm-linux-gnueabihf（Linux 应用）
# -----------------------------------------------------------------------------
# 添加裸机可执行目标：<name>.elf，并在 POST_BUILD 生成 <name>.hex
#
# 必选参数:
#   LINKER_SCRIPT  链接脚本路径
# 常用参数:
#   SOURCES        源文件列表（.c / .s）
#   INCLUDE_DIRS   头文件目录
#   MCU_FLAGS      默认 "-mcpu=cortex-m3 -mthumb"
function(embed_mcu_add_executable target_name)
    set(options)
    set(oneValueArgs LINKER_SCRIPT MCU_FLAGS)
    set(multiValueArgs SOURCES INCLUDE_DIRS)
    cmake_parse_arguments(MCU "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT MCU_LINKER_SCRIPT)
        message(FATAL_ERROR "LINKER_SCRIPT is required")
    endif()

    if(NOT MCU_MCU_FLAGS)
        set(MCU_MCU_FLAGS "-mcpu=cortex-m3 -mthumb")
    endif()

    separate_arguments(MCU_FLAGS_LIST NATIVE_COMMAND "${MCU_MCU_FLAGS}")

    # 段级链接优化：配合 -Wl,--gc-sections 去除未引用代码
    add_compile_options(
        ${MCU_FLAGS_LIST}
        -ffunction-sections
        -fdata-sections
    )

    if(MCU_INCLUDE_DIRS)
        include_directories(${MCU_INCLUDE_DIRS})
    endif()

    add_executable(${target_name}.elf ${MCU_SOURCES})

    target_compile_options(${target_name}.elf PRIVATE
        $<$<CONFIG:Debug>:-O0 -g3>
        $<$<CONFIG:Release>:-Os>
    )

    target_link_options(${target_name}.elf PRIVATE
        ${MCU_FLAGS_LIST}
        -T ${MCU_LINKER_SCRIPT}
        -Wl,-Map=${target_name}.map
        -Wl,--gc-sections
    )

    # objcopy 与 gcc 同目录（Windows 带 .exe）
    get_filename_component(_embed_tool_bin "${CMAKE_C_COMPILER}" DIRECTORY)
    set(_embed_objcopy "${_embed_tool_bin}/arm-none-eabi-objcopy")
    if(NOT EXISTS "${_embed_objcopy}" AND EXISTS "${_embed_objcopy}.exe")
        set(_embed_objcopy "${_embed_objcopy}.exe")
    endif()
    if(NOT EXISTS "${_embed_objcopy}")
        message(FATAL_ERROR "arm-none-eabi-objcopy not found next to compiler: ${_embed_objcopy}")
    endif()

    # ELF → HEX：构建完成后由 objcopy 生成（OpenOCD flash-openocd 使用 .hex）
    # 等价命令: arm-none-eabi-objcopy -O ihex f103-blink.elf f103-blink.hex
    add_custom_command(TARGET ${target_name}.elf POST_BUILD
        COMMAND "${_embed_objcopy}" -O ihex ${target_name}.elf ${target_name}.hex
        COMMENT "Generating ${target_name}.hex"
    )
endfunction()
