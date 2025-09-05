cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CheckLanguage)
include(CheckTypeSize)
include(ExternalProject)
# include(ProcessTarget)
# include(BuildConfiguration)

# Build configurations
set(TARGET_ARCH "i686" CACHE STRING "Target CPU architecture")
set(TARGET_BOARD "pc" CACHE STRING "Target board type")
set(TARGET_FIRMWARE "bios" CACHE STRING "Target firmware")
set(CONFIG_PRESET "default" CACHE STRING "Preset name of build configuration")

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/${TARGET_ARCH}/Toolchain.cmake")

project("emos")

# Language configurations
enable_language(C ASM)

# Load build configurations
# load_build_configuration(TARGET CONFIG_PRESET)

# Subdirectories
add_subdirectory(tools)
add_subdirectory(boot)
add_subdirectory(kernel)
add_subdirectory(sdk)
add_subdirectory(modules)
add_subdirectory(bin)
