cmake_minimum_required(VERSION 3.13)

add_subdirectory("${TARGET_BOARD}")

if (NOT I686_EXCLUDE_CPU_MODE)
    target_sources(bootloader PRIVATE cpu_mode.S)
endif()
target_sources(bootloader PRIVATE setjmp.S)
target_include_directories(bootloader PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
