cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)
set(CMAKE_TOOLCHAIN_FILE "${ROOT_SOURCE_DIR}/cmake/${TARGET_ARCH}/Toolchain.cmake")

project("eboot")

# Language configurations
enable_language(C CXX ASM)

# Compiler Tests

# config.h
configure_file("config.h.in" "config.h")

# Dependencies
find_package(Python3 COMPONENTS Interpreter)

# Target Configurations
add_executable(bootloader)
set_target_properties(bootloader PROPERTIES SUFFIX ".elf")

# Compile Options
target_compile_features(bootloader PUBLIC c_std_11)
target_compile_features(bootloader PUBLIC cxx_std_14)
target_compile_options(bootloader PUBLIC 
    $<$<COMPILE_LANGUAGE:C,CXX>:
        -Werror
        -Wall
        -Wunused-result
        -Wno-unused-function
    >
)
target_link_libraries(bootloader gcc)

target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(bootloader PRIVATE "${CMAKE_BINARY_DIR}")

target_compile_definitions(bootloader PRIVATE CRC32_FAST=1)

# Additional Targets
add_custom_target(bootloader_symbol_table ALL
    COMMAND ${CMAKE_NM} "$<TARGET_FILE:bootloader>" > bootloader.map
    DEPENDS bootloader
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Creating symbol table"
)

add_custom_command(
    OUTPUT bootloader_exported_symbol_table.stamp
    COMMAND ${CMAKE_COMMAND} -E touch bootloader_exported_symbol_table.stamp
    COMMAND ${Python3_EXECUTABLE} "${ROOT_SOURCE_DIR}/tools/mksymtab/mksymtab.py" "${CMAKE_CURRENT_SOURCE_DIR}/exported_symbols.txt" "exported_symbols.c"
    DEPENDS exported_symbols.txt
)
add_custom_target(bootloader_exported_symbol_table ALL
    DEPENDS bootloader_exported_symbol_table.stamp
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Creating exported symbol table"
)
add_dependencies(bootloader bootloader_exported_symbol_table)
target_sources(bootloader PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/exported_symbols.c")
set_source_files_properties("${CMAKE_CURRENT_BINARY_DIR}/exported_symbols.c" PROPERTIES GENERATED TRUE)

add_custom_target(bootloader_font ALL
    COMMAND ${Python3_EXECUTABLE} "${ROOT_SOURCE_DIR}/tools/hex2bfn/hex2bfn.py" "${CMAKE_CURRENT_SOURCE_DIR}/unifont.hex" "unifont.bfn"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Generating font data"
)

# libraries
add_subdirectory(lib)
target_link_libraries(bootloader uacpi.lib zlib.lib openlibm.lib)

# Subdirectories
add_subdirectory(arch)

if (NOT "${TARGET_FIRMWARE}" STREQUAL "uefi")
    add_subdirectory(acpi)
    add_subdirectory(core)
    add_subdirectory(debug)
    add_subdirectory(device)
    add_subdirectory(elf)
    add_subdirectory(encoding)
    add_subdirectory(filesystem)
    add_subdirectory(font)
    add_subdirectory(json)
    add_subdirectory(mm)
    add_subdirectory(module)
    add_subdirectory(pci)
    add_subdirectory(stdc)
    add_subdirectory(shell)
    add_subdirectory(modules)
endif()
