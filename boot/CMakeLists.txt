cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)

# Target Configurations
add_executable(bootloader)
set_target_properties(bootloader PROPERTIES SUFFIX ".elf")

# Compiler Tests

# config.h
configure_file("config.h.in" "config.h")

# Dependencies

# Compile Options
target_compile_features(bootloader PUBLIC c_std_11)
target_compile_features(bootloader PUBLIC cxx_std_14)
target_compile_options(bootloader PUBLIC 
    $<$<COMPILE_LANGUAGE:C,CXX>:
        #-Werror
        -Wall
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        #-pedantic
        #-pedantic-errors
    >)
target_link_libraries(bootloader gcc)

target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(bootloader PRIVATE "${CMAKE_BINARY_DIR}")

# Subdirectories
add_subdirectory(arch)
add_subdirectory(bus)
add_subdirectory(term)
add_subdirectory(video)
add_subdirectory(disk)
add_subdirectory(fs)
add_subdirectory(stdc)
add_subdirectory(lib)
add_subdirectory(core)

# Additional Targets

