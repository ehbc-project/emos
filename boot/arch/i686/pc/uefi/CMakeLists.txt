cmake_minimum_required(VERSION 3.13)

set(BUILD_CDIMAGE TRUE CACHE BOOL "Build CD image")

set(I686_EXCLUDE_CPU_MODE TRUE PARENT_SCOPE)

target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(bootloader PRIVATE "${CMAKE_BINARY_DIR}")
target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/gnu-efi/inc")
target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/gnu-efi/inc/ia32")

add_dependencies(bootloader gnu-efi)

set(CMAKE_LINKER ${CMAKE_LD})

target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_options(bootloader PUBLIC
    $<$<COMPILE_LANGUAGE:C,CXX>:
        -fpic
        -fno-stack-protector
        -fno-stack-check
        -fshort-wchar
        -mno-red-zone
        -maccumulate-outgoing-args
        -DEFI_FUNCTION_WRAPPER
    >
)
target_sources(bootloader PRIVATE init.c)
target_link_options(bootloader PUBLIC
    -T "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/gnu-efi/gnuefi/elf_ia32_efi.lds"
    -Wl,-shared
    -Wl,-Bsymbolic
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/gnu-efi/ia32/gnuefi/crt0-efi-ia32.o"
)
set_target_properties(bootloader PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/gnu-efi/gnuefi/elf_ia32_efi.lds")
target_link_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/gnu-efi/ia32/lib" "${CMAKE_CURRENT_SOURCE_DIR}/../../../../lib/gnu-efi/ia32/gnuefi")
target_link_libraries(bootloader gnuefi efi)

add_custom_target(bootloader_efibin ALL
    COMMAND objcopy -j .text -j .sdata -j .data -j .rodata -j .dynamic -j .dynsym -j .rel -j .rela -j .rel.* -j .rela.* -j .reloc -j .areloc --target efi-app-ia32 --subsystem=10 "${CMAKE_CURRENT_BINARY_DIR}/../../../../bootloader.elf" bootloader.efi
    DEPENDS bootloader
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Creating bootloader EFI binary")
