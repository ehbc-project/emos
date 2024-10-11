cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

include(CheckLanguage)
include(CheckTypeSize)
include(ProcessTarget)
include(BuildConfiguration)

project("emos")

# Build configurations
set(TARGET_ARCH "native" CACHE STRING "Target CPU architecture")
set(TARGET_BOARD "native" CACHE STRING "Target board type")
set(CONFIG_PRESET "default" CACHE STRING "Preset name of build configuration")

# Target
string(REPLACE "-" ";" TARGET ${TARGET})
list(GET TARGET 0 TARGET_ARCH)
list(GET TARGET 1 TARGET_BOARD)

# Language configurations
enable_language(C ASM)

# Load build configurations
load_build_configuration(TARGET CONFIG_PRESET)

# Subdirectories
add_subdirectory(boot)
add_subdirectory(kernel)
add_subdirectory(sdk)
add_subdirectory(modules)
add_subdirectory(bin)
