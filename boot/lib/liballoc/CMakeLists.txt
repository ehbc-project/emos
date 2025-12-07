cmake_minimum_required(VERSION 3.13)

target_sources(bootloader PRIVATE liballoc.c platform.c)
target_include_directories(bootloader PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
