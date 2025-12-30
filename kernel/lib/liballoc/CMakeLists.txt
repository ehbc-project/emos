cmake_minimum_required(VERSION 3.13)

target_sources(kernel PRIVATE liballoc.c platform.c)
target_include_directories(kernel PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
