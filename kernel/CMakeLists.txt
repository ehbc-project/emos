cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)

# Compiler Tests

# config.h
configure_file("config.h.in" "config.h")

# Dependencies
find_package(Python3 COMPONENTS Interpreter)

# Target Configurations
add_executable(kernel)
set_target_properties(kernel PROPERTIES SUFFIX ".elf")

# Compile Options
target_compile_features(kernel PUBLIC c_std_11)
target_compile_features(kernel PUBLIC cxx_std_14)
target_compile_options(kernel PUBLIC 
    $<$<COMPILE_LANGUAGE:C,CXX>:
        -Werror
        -Wall
        -Wno-unused-function
        -pedantic
        -pedantic-errors
    >
)
target_link_libraries(kernel gcc)

target_include_directories(kernel PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(kernel PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(kernel PRIVATE "${CMAKE_BINARY_DIR}")

target_compile_definitions(kernel PRIVATE CRC32_FAST=1)

# Additional Targets
add_custom_target(kernel_symbol_table ALL
    COMMAND ${CMAKE_NM} "$<TARGET_FILE:kernel>" > kernel.map
    DEPENDS kernel
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Creating symbol table"
)

# Subdirectories
add_subdirectory(lib)
add_subdirectory(arch)
# add_subdirectory(bus)
# add_subdirectory(device)
# add_subdirectory(filesystem)
add_subdirectory(init)
add_subdirectory(log)
add_subdirectory(mm)
add_subdirectory(stdc)
add_subdirectory(thread)
