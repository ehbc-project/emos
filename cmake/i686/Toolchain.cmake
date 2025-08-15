set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR m68k)

set(CMAKE_SYSROOT)

find_program(CMAKE_C_COMPILER
    "i686-elf-gcc"
    HINTS "/usr" "/usr/local" "/opt/homebrew" ENV PATH
    REQUIRED)
set(CMAKE_C_COMPILER_TARGET     i686-elf)
set(CMAKE_C_FLAGS               "${CMAKE_C_FLAGS} -ffreestanding -nostdlib -march=i686")

find_program(CMAKE_CXX_COMPILER
    "i686-elf-g++"
    HINTS "/usr" "/usr/local" "/opt/homebrew" ENV PATH
    REQUIRED)
set(CMAKE_CXX_COMPILER_TARGET   i686-elf)
set(CMAKE_CXX_FLAGS             "${CMAKE_CXX_FLAGS} -ffreestanding -nostdlib -march=i686")

find_program(CMAKE_ASM_COMPILER
    "i686-elf-as"
    HINTS "/usr" "/usr/local" "/opt/homebrew" ENV PATH
    REQUIRED)
set(CMAKE_ASM_COMPILER_TARGET   i686-elf)

set(_BINUTILS_LIST AR;NM;OBJCOPY;OBJDUMP;RANLIB;READELF;STRIP)

foreach(TOOL_NAME IN LISTS _BINUTILS_LIST)
    string(TOLOWER ${TOOL_NAME} _TOOL_BIN_NAME)
    find_program(CMAKE_${TOOL_NAME}
        "i686-elf-${_TOOL_BIN_NAME}"
        HINTS "/usr" "/usr/local" "/opt/homebrew" ENV PATH
        REQUIRED)
    unset(_TOOL_BIN_NAME)
endforeach()

unset(_BINUTILS_LIST)

