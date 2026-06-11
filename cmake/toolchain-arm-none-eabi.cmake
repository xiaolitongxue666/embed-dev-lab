# -----------------------------------------------------------------------------
# ARM 裸机交叉编译工具链（Generic / arm-none-eabi-gcc）
# 由 CMakePresets 通过 CMAKE_TOOLCHAIN_FILE 引用
# 前置: ./scripts/install-tools.sh && ./scripts/setup-path.sh
# -----------------------------------------------------------------------------

# 目标为裸机，非 Linux/Windows 主机
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# try_compile 仅生成静态库，避免链接主机库失败
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(_EMBED_TOOLCHAIN_PREFIX "arm-none-eabi-")

# 在 PATH 或 Windows 默认安装目录查找工具链可执行文件
function(_embed_find_tool name out_var)
    find_program(_found "${name}" PATHS ENV PATH)
    if(_found)
        set(${out_var} "${_found}" PARENT_SCOPE)
        return()
    endif()

    if(WIN32)
        file(GLOB _arm_bins
            "C:/Program Files/Arm GNU Toolchain arm-none-eabi/*/bin/${name}.exe"
            "C:/Program Files (x86)/Arm GNU Toolchain arm-none-eabi/*/bin/${name}.exe"
        )
        if(_arm_bins)
            list(GET _arm_bins 0 _found)
            set(${out_var} "${_found}" PARENT_SCOPE)
        endif()
    endif()
endfunction()

_embed_find_tool("${_EMBED_TOOLCHAIN_PREFIX}gcc" CMAKE_C_COMPILER)
_embed_find_tool("${_EMBED_TOOLCHAIN_PREFIX}g++" CMAKE_CXX_COMPILER)
_embed_find_tool("${_EMBED_TOOLCHAIN_PREFIX}gcc" CMAKE_ASM_COMPILER)
_embed_find_tool("${_EMBED_TOOLCHAIN_PREFIX}objcopy" EMBED_OBJCOPY)
_embed_find_tool("${_EMBED_TOOLCHAIN_PREFIX}size" EMBED_SIZE)

if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR "arm-none-eabi-gcc not found. Run ./scripts/install-tools.sh and ./scripts/setup-path.sh")
endif()

if(NOT EMBED_OBJCOPY)
    message(FATAL_ERROR "arm-none-eabi-objcopy not found. Run ./scripts/install-tools.sh and ./scripts/setup-path.sh")
endif()

# objcopy 用于 POST_BUILD 将 .elf 转为 .hex（见 cmake/mcu-config.cmake）
set(CMAKE_OBJCOPY "${EMBED_OBJCOPY}" CACHE FILEPATH "ARM objcopy" FORCE)

set(CMAKE_EXECUTABLE_FORMAT ELF)
# 裸机链接：nosys + 不提供默认 startup（由模块 startup.s 提供）
set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs -nostartfiles")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS_INIT}" CACHE INTERNAL "")

# 交叉编译：仅在目标 sysroot 中查找库/头文件，程序仍在主机 PATH 查找
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# 供 clangd / IDE 使用 compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)
