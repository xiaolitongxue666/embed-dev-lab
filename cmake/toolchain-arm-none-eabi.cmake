set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(_EMBED_TOOLCHAIN_PREFIX "arm-none-eabi-")

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

set(CMAKE_OBJCOPY "${EMBED_OBJCOPY}" CACHE FILEPATH "ARM objcopy" FORCE)

set(CMAKE_EXECUTABLE_FORMAT ELF)
set(CMAKE_EXE_LINKER_FLAGS_INIT "--specs=nosys.specs -nostartfiles")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS_INIT}" CACHE INTERNAL "")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON CACHE BOOL "" FORCE)
