set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR m68k)

set(CMAKE_SYSROOT)

set(TOOLCHAIN_PREFIX "m68k-unknown-elf-")

find_program(CMAKE_C_COMPILER
    "${TOOLCHAIN_PREFIX}gcc"
    HINTS "/usr" "/usr/local" "/opt/homebrew" ENV PATH
    REQUIRED)
set(CMAKE_C_COMPILER_TARGET     m68k-unknown-elf)
set(CMAKE_C_FLAGS               "${CMAKE_C_FLAGS} -ffreestanding -nostdlib -march=68030 -m68881")

find_program(CMAKE_CXX_COMPILER
    "${TOOLCHAIN_PREFIX}g++"
    HINTS "/usr" "/usr/local" "/opt/homebrew" ENV PATH
    REQUIRED)
set(CMAKE_CXX_COMPILER_TARGET   m68k-unknown-elf)
set(CMAKE_CXX_FLAGS             "${CMAKE_CXX_FLAGS} -ffreestanding -nostdlib -march=68030 -m68881")

set(CMAKE_ASM_COMPILER          "${CMAKE_C_COMPILER}")
set(CMAKE_ASM_COMPILER_TARGET   m68k-unknown-elf)
set(CMAKE_ASM_FLAGS             "${CMAKE_ASM_FLAGS} -ffreestanding -nostdlib -march=68030 -m68881")

set(_BINUTILS_LIST LD;AR;NM;OBJCOPY;OBJDUMP;RANLIB;READELF;STRIP)

foreach(TOOL_NAME IN LISTS _BINUTILS_LIST)
    string(TOLOWER ${TOOL_NAME} _TOOL_BIN_NAME)
    find_program(CMAKE_${TOOL_NAME}
        "${TOOLCHAIN_PREFIX}${_TOOL_BIN_NAME}"
        HINTS "/usr" "/usr/local" "/opt/homebrew" ENV PATH
        REQUIRED)
    unset(_TOOL_BIN_NAME)
endforeach()

unset(_BINUTILS_LIST)
