cmake_minimum_required(VERSION 3.13)
cmake_policy(SET CMP0076 NEW)

set(MODULE_NAME bootemos)

# Compiler Tests

# Target Configurations
add_library(bootloader_module_${MODULE_NAME} OBJECT main.c)
add_custom_target(bootloader_module_${MODULE_NAME}_mod ALL
    COMMAND
        ${CMAKE_LD}
            -shared -fno-plt -z now -Bsymbolic
            --emit-relocs --export-dynamic --allow-shlib-undefined
            $<TARGET_OBJECTS:bootloader_module_${MODULE_NAME}>
            -o ${MODULE_NAME}.elf
    COMMAND
        ${CMAKE_OBJCOPY}
            --add-section .note.eboot=${CMAKE_CURRENT_SOURCE_DIR}/module_info.bin
            --set-section-flags .note.eboot=contents,noload,readonly
            ${MODULE_NAME}.elf ${MODULE_NAME}.mod
    COMMAND_EXPAND_LISTS
    DEPENDS bootloader_module_${MODULE_NAME}
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Linking target bootloader_module_${MODULE_NAME}"
    VERBATIM
)

# Compile Options
target_compile_features(bootloader_module_${MODULE_NAME} PUBLIC c_std_11)
target_compile_features(bootloader_module_${MODULE_NAME} PUBLIC cxx_std_14)
target_compile_options(bootloader_module_${MODULE_NAME} PUBLIC 
    $<$<COMPILE_LANGUAGE:C,CXX>:
        -fpic
        -fno-plt
        -Werror
        -Wall
        -Wunused-result
        -Wno-unused-function
    >
)

target_include_directories(bootloader_module_${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(bootloader_module_${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(bootloader_module_${MODULE_NAME} PRIVATE "${CMAKE_BINARY_DIR}")
target_include_directories(bootloader_module_${MODULE_NAME} PRIVATE "${ROOT_SOURCE_DIR}/common/include")

get_target_property(BOOTLOADER_INCLUDES bootloader INCLUDE_DIRECTORIES)
target_include_directories(bootloader_module_${MODULE_NAME} PRIVATE ${BOOTLOADER_INCLUDES})
