cmake_minimum_required(VERSION 3.13)

set(BUILD_FDIMAGE TRUE CACHE BOOL "Build floppy disk image")
set(BUILD_CDIMAGE TRUE CACHE BOOL "Build CD image")

if (BUILD_FDIMAGE OR BUILD_CDIMAGE)
    add_executable(fdboot)
    set_target_properties(fdboot PROPERTIES SUFFIX ".elf")
    target_compile_features(fdboot PUBLIC c_std_11)
    target_compile_features(fdboot PUBLIC cxx_std_14)
    target_compile_options(fdboot PUBLIC 
        $<$<COMPILE_LANGUAGE:C,CXX>:
            -Werror
            -Wall
            -Wno-unused-function
            -Wno-unused-variable
            -Wno-unused-but-set-variable
            -pedantic
            -pedantic-errors
        >)
    target_link_options(fdboot PUBLIC -T "${CMAKE_CURRENT_SOURCE_DIR}/ldscript_vbr.x")
    set_target_properties(fdboot PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ldscript_vbr.x")
    target_sources(fdboot PRIVATE fdboot.S)

    add_custom_target(fdboot_bin ALL
        COMMAND ${CMAKE_OBJCOPY} -O binary "$<TARGET_FILE:fdboot>" fdboot.bin
        DEPENDS fdboot
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Creating floppy VBR")
endif()

target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(bootloader PRIVATE "${CMAKE_BINARY_DIR}")

target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(bootloader PRIVATE start.S chainload.S reboot.c poweroff.c halt.S bootinfo.c init.c)
target_link_options(bootloader PUBLIC -T "${CMAKE_CURRENT_SOURCE_DIR}/ldscript.x")
set_target_properties(bootloader PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ldscript.x")

add_subdirectory(acpi)
add_subdirectory(bios)
add_subdirectory(pci)
add_subdirectory(vbe)

if (BUILD_FDIMAGE OR BUILD_CDIMAGE)
    add_custom_target(bootloader_fdimage ALL
        COMMAND ${CMAKE_OBJCOPY} -O binary "$<TARGET_FILE:bootloader>" bootloader.bin
        COMMAND dd if=/dev/zero of="${CMAKE_BINARY_DIR}/boot/floppy.img" bs=1024 count=1440
        COMMAND mformat -i "${CMAKE_BINARY_DIR}/boot/floppy.img"
        COMMAND mcopy -i "${CMAKE_BINARY_DIR}/boot/floppy.img" "${CMAKE_CURRENT_BINARY_DIR}/bootloader.bin" ::/BOOTLDR.BIN
        COMMAND mcopy -s -i "${CMAKE_BINARY_DIR}/boot/floppy.img" "${CMAKE_SOURCE_DIR}/boot/config" ::/CONFIG
        COMMAND mcopy -i "${CMAKE_BINARY_DIR}/boot/floppy.img" "${CMAKE_BINARY_DIR}/boot/bootloader.map" ::/BOOTLDR.MAP
        COMMAND mcopy -i "${CMAKE_BINARY_DIR}/boot/floppy.img" "${CMAKE_BINARY_DIR}/boot/unifont.bfn" ::/UNIFONT.BFN
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/injectvbr/injectvbr.py" "${CMAKE_CURRENT_BINARY_DIR}/fdboot.bin" "${CMAKE_BINARY_DIR}/boot/floppy.img"
        DEPENDS bootloader
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Creating bootloader floppy image")
endif()

if (BUILD_CDIMAGE)
    add_custom_target(bootloader_cdimage ALL
        COMMAND mkisofs -o "${CMAKE_BINARY_DIR}/boot/cdrom.iso" -b floppy.img -c boot.catalog "${CMAKE_BINARY_DIR}/boot/floppy.img"
        DEPENDS bootloader
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Creating bootloader cd image")
endif()

# add_custom_target(bootloader_hdimage ALL
#     COMMAND dd if=/dev/zero of="${CMAKE_BINARY_DIR}/boot/hd.img" bs=1048576 count=16
#     COMMAND "${CMAKE_BINARY_DIR}/tools/mkfs.afs/mkfs.afs" -b "${CMAKE_CURRENT_BINARY_DIR}/fdboot.bin" -j 2M -l "HDBOOT" -N 2 -o "emos" -r 8 -s 1 -u 00000000-0000-0000-0000-000000000000 -iv "${CMAKE_BINARY_DIR}/boot/hd.img"
#     DEPENDS bootloader mkfs.afs-host
#     WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
#     COMMENT "Creating bootloader hd image")
