cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CheckLanguage)
include(CheckTypeSize)
include(ExternalProject)
# include(ProcessTarget)
# include(BuildConfiguration)

project("emos")

# Load build configurations
# load_build_configuration(TARGET CONFIG_PRESET)

# Subdirectories
add_subdirectory(tools)  # host tools

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

if ("${TARGET_ARCH}" STREQUAL "amd64")
    set(EBOOT_TARGET_ARCH i686)
else()
    set(EBOOT_TARGET_ARCH "${TARGET_ARCH}")
endif()

ExternalProject_Add(eboot
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/boot"
    BINARY_DIR "${CMAKE_BINARY_DIR}/boot"
    CMAKE_ARGS
        -DTARGET_ARCH=${EBOOT_TARGET_ARCH}
        -DTARGET_BOARD=${TARGET_BOARD}
        -DTARGET_FIRMWARE=${TARGET_FIRMWARE}
        -DROOT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DROOT_BINARY_DIR=${CMAKE_BINARY_DIR}
    BUILD_ALWAYS 1
    INSTALL_COMMAND ""
)

ExternalProject_Add(ekern
    SOURCE_DIR "${CMAKE_SOURCE_DIR}/kernel"
    BINARY_DIR "${CMAKE_BINARY_DIR}/kernel"
    CMAKE_ARGS
        -DTARGET_ARCH=${TARGET_ARCH}
        -DTARGET_BOARD=${TARGET_BOARD}
        -DTARGET_FIRMWARE=${TARGET_FIRMWARE}
        -DROOT_SOURCE_DIR=${CMAKE_SOURCE_DIR}
        -DROOT_BINARY_DIR=${CMAKE_BINARY_DIR}
    BUILD_ALWAYS 1
    INSTALL_COMMAND ""
)
