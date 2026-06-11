# Reusable MCU build helpers for bare-metal ARM modules.

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

    get_filename_component(_embed_tool_bin "${CMAKE_C_COMPILER}" DIRECTORY)
    set(_embed_objcopy "${_embed_tool_bin}/arm-none-eabi-objcopy")
    if(NOT EXISTS "${_embed_objcopy}" AND EXISTS "${_embed_objcopy}.exe")
        set(_embed_objcopy "${_embed_objcopy}.exe")
    endif()
    if(NOT EXISTS "${_embed_objcopy}")
        message(FATAL_ERROR "arm-none-eabi-objcopy not found next to compiler: ${_embed_objcopy}")
    endif()

    add_custom_command(TARGET ${target_name}.elf POST_BUILD
        COMMAND "${_embed_objcopy}" -O ihex ${target_name}.elf ${target_name}.hex
        COMMENT "Generating ${target_name}.hex"
    )
endfunction()
