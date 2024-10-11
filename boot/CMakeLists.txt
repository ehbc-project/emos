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
        -Werror
        -Wall
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -pedantic
        -pedantic-errors>)
target_link_libraries(bootloader gcc)

target_include_directories(bootloader PUBLIC "${CMAKE_SOURCE_DIR}/include")
target_include_directories(bootloader PRIVATE "${CMAKE_SOURCE_DIR}/src")
target_include_directories(bootloader PRIVATE "${CMAKE_BINARY_DIR}")

# Subdirectories
add_subdirectory(arch)
add_subdirectory(common)

# Additional Targets
add_custom_target(bootloader_image ALL
    COMMAND ${CMAKE_OBJCOPY} -O binary "$<TARGET_FILE:bootloader>" bootloader.bin
    DEPENDS bootloader
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Creating bootloader image")
