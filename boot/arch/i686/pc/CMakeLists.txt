cmake_minimum_required(VERSION 3.13)

add_executable(bootsector)
set_target_properties(bootsector PROPERTIES SUFFIX ".elf")
target_compile_features(bootsector PUBLIC c_std_11)
target_compile_features(bootsector PUBLIC cxx_std_14)
target_compile_options(bootsector PUBLIC 
    $<$<COMPILE_LANGUAGE:C,CXX>:
        -Werror
        -Wall
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -pedantic
        -pedantic-errors
    >)
target_link_options(bootsector PUBLIC -T "${CMAKE_CURRENT_SOURCE_DIR}/ldscript_vbr.x")
set_target_properties(bootsector PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ldscript_vbr.x")
target_sources(bootsector PRIVATE flopboot.S)

add_custom_target(bootsector_bin ALL
    COMMAND ${CMAKE_OBJCOPY} -O binary "$<TARGET_FILE:bootsector>" bootsector.bin
    DEPENDS bootsector
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Creating bootsector image")
    

target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(bootloader PRIVATE "${CMAKE_BINARY_DIR}")

target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(bootloader PRIVATE start.S chainload.S bios/bioscall.S bootinfo.c bios/disk.c init.c bios/keyboard.c bios/video.c bios/mem.c pci/cfgspace.c acpi/rsdp.c)
target_link_options(bootloader PUBLIC -T "${CMAKE_CURRENT_SOURCE_DIR}/ldscript.x")
set_target_properties(bootloader PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ldscript.x")

add_custom_target(bootloader_image ALL
    COMMAND ${CMAKE_OBJCOPY} -O binary "$<TARGET_FILE:bootloader>" bootloader.bin
    COMMAND dd if=/dev/zero of="${CMAKE_CURRENT_BINARY_DIR}/floppy.img" bs=1024 count=1440
    COMMAND mformat -i "${CMAKE_CURRENT_BINARY_DIR}/floppy.img"
    COMMAND mcopy -i "${CMAKE_CURRENT_BINARY_DIR}/floppy.img" "${CMAKE_CURRENT_BINARY_DIR}/bootloader.bin" ::/BOOTLDR.BIN
    COMMAND mcopy -s -i "${CMAKE_CURRENT_BINARY_DIR}/floppy.img" "${CMAKE_SOURCE_DIR}/boot/config" ::/config
    COMMAND python "${CMAKE_SOURCE_DIR}/tools/injectvbr/injectvbr.py" "${CMAKE_CURRENT_BINARY_DIR}/bootsector.bin" "${CMAKE_CURRENT_BINARY_DIR}/floppy.img"
    DEPENDS bootloader
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Creating bootloader image")
