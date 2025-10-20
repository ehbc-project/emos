cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)

project(afsimg)

# Compiler Tests

# Dependencies

# Target Configurations
add_executable(afsimg)

# Compile Options
target_compile_features(afsimg PUBLIC c_std_11)
target_compile_features(afsimg PUBLIC cxx_std_14)
target_compile_options(afsimg PUBLIC 
    $<$<COMPILE_LANGUAGE:C,CXX>:
        -Werror
        -Wall
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -Wno-unused-private-field
    >
)
target_sources(afsimg PRIVATE afs.cpp crc32.c image.cpp main.cpp)
target_compile_definitions(afsimg PRIVATE CRC32_FAST)

target_include_directories(afsimg PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(afsimg PRIVATE "${CMAKE_BINARY_DIR}")

# Additional Targets

# Subdirectories
