cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CheckLanguage)
include(CheckTypeSize)
include(ExternalProject)
# include(ProcessTarget)
# include(BuildConfiguration)

# Build configurations
set(TARGET "i686-pc-bios" CACHE STRING "Target triple (arch-board-firmware)")
set(CONFIG_PRESET "default" CACHE STRING "Preset name of build configuration")

string(REPLACE "-" ";" TARGET_TEMP ${TARGET})
list(LENGTH TARGET_TEMP TARGET_TEMP_LEN)
if (NOT TARGET_TEMP_LEN EQUAL 3)
    message(FATAL_ERROR "Invalid target format")
endif()
list(GET TARGET_TEMP 0 TARGET_ARCH)
list(GET TARGET_TEMP 1 TARGET_BOARD)
list(GET TARGET_TEMP 2 TARGET_FIRMWARE)
unset(TARGET_TEMP)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/${TARGET_ARCH}/Toolchain.cmake")

project("emos")

# Language configurations
enable_language(C CXX ASM)

# Load build configurations
# load_build_configuration(TARGET CONFIG_PRESET)

# Subdirectories
add_subdirectory(tools)
add_subdirectory(boot)
add_subdirectory(kernel)
add_subdirectory(packages)
