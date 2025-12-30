cmake_minimum_required(VERSION 3.13)

add_subdirectory("${TARGET_BOARD}")

target_sources(kernel PUBLIC setjmp.S)
target_include_directories(kernel PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(kernel PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
