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
        >
    )
    target_link_options(fdboot PUBLIC -T "${CMAKE_CURRENT_SOURCE_DIR}/ldscript_vbr.x")
    set_target_properties(fdboot PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ldscript_vbr.x")
    target_sources(fdboot PRIVATE fdboot.S)

    add_custom_target(fdboot_bin ALL
        COMMAND ${CMAKE_OBJCOPY} -O binary "$<TARGET_FILE:fdboot>" fdboot.bin
        DEPENDS fdboot
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Creating floppy VBR"
    )


    add_executable(mbrboot)
    set_target_properties(mbrboot PROPERTIES SUFFIX ".elf")
    target_compile_features(mbrboot PUBLIC c_std_11)
    target_compile_features(mbrboot PUBLIC cxx_std_14)
    target_compile_options(mbrboot PUBLIC 
        $<$<COMPILE_LANGUAGE:C,CXX>:
            -Werror
            -Wall
            -Wno-unused-function
            -Wno-unused-variable
            -Wno-unused-but-set-variable
            -pedantic
            -pedantic-errors
        >
    )
    target_link_options(mbrboot PUBLIC -T "${CMAKE_CURRENT_SOURCE_DIR}/ldscript_vbr.x")
    set_target_properties(mbrboot PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ldscript_vbr.x")
    target_sources(mbrboot PRIVATE mbrboot.S)

    add_custom_target(mbrboot_bin ALL
        COMMAND ${CMAKE_OBJCOPY} -O binary "$<TARGET_FILE:mbrboot>" mbrboot.bin
        DEPENDS mbrboot
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Creating floppy VBR"
    )
endif()

add_executable(stage1)
set_target_properties(stage1 PROPERTIES SUFFIX ".elf")

target_compile_features(stage1 PUBLIC c_std_11)
target_compile_features(stage1 PUBLIC cxx_std_14)
target_compile_options(stage1 PUBLIC 
    $<$<COMPILE_LANGUAGE:C,CXX>:
        -Werror
        -Wall
        -Wno-unused-function
        -Wno-unused-variable
        -Wno-unused-but-set-variable
        -pedantic
        -pedantic-errors
    >
)
target_link_libraries(stage1 gcc)

target_include_directories(stage1 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(stage1 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(stage1 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../..")
target_include_directories(stage1 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../include")
target_include_directories(stage1 PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/../../../../include")
target_include_directories(stage1 PRIVATE "${CMAKE_BINARY_DIR}")

target_sources(stage1 PRIVATE stage1.S stage1.c chainload.S ../../cpu_mode.S ../../io.S ../../interrupt.S ../../breakpoint.S pic.c)
target_link_options(stage1 PUBLIC -T "${CMAKE_CURRENT_SOURCE_DIR}/ldscript_stage1.x")
set_target_properties(stage1 PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ldscript_stage1.x")

add_custom_target(stage1_bin ALL
    COMMAND ${CMAKE_OBJCOPY} -O binary "$<TARGET_FILE:stage1>" stage1.bin
    DEPENDS stage1
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Creating stage1 binary"
)

set(I686_EXCLUDE_CPU_MODE TRUE PARENT_SCOPE)
add_dependencies(bootloader stage1)
target_link_options(bootloader PUBLIC -Wl,--just-symbols=$<TARGET_FILE:stage1>)
set_target_properties(bootloader PROPERTIES LINK_DEPENDS stage1)

target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_include_directories(bootloader PRIVATE "${CMAKE_BINARY_DIR}")

target_sources(bootloader PRIVATE start.S reboot.c poweroff.c halt.S isr.c isr.S init.c)
target_link_options(bootloader PUBLIC -T "${CMAKE_CURRENT_SOURCE_DIR}/ldscript.x")
set_target_properties(bootloader PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ldscript.x")

add_subdirectory(acpi)
add_subdirectory(bios)
add_subdirectory(pci)
add_subdirectory(vbe)

add_custom_target(bootloader_execbin ALL
    COMMAND ${CMAKE_OBJCOPY} -O binary "$<TARGET_FILE:bootloader>" bootloader.bin
    DEPENDS bootloader
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    COMMENT "Creating bootloader executable binary"
)

if (BUILD_FDIMAGE OR BUILD_CDIMAGE)
    add_custom_target(bootloader_fdimage ALL
        COMMAND dd if=/dev/zero of="${CMAKE_BINARY_DIR}/boot/floppy.img" bs=1024 count=1440
        COMMAND mformat -i "${CMAKE_BINARY_DIR}/boot/floppy.img" -R 18 -B "${CMAKE_CURRENT_BINARY_DIR}/fdboot.bin"
        COMMAND mcopy -i "${CMAKE_BINARY_DIR}/boot/floppy.img" "${CMAKE_CURRENT_BINARY_DIR}/bootloader.bin" ::/BOOTLDR.X86
        COMMAND mcopy -s -i "${CMAKE_BINARY_DIR}/boot/floppy.img" "${CMAKE_SOURCE_DIR}/boot/config" ::/CONFIG
        COMMAND mcopy -i "${CMAKE_BINARY_DIR}/boot/floppy.img" "${CMAKE_BINARY_DIR}/boot/bootloader.map" ::/BOOTLDR.MAP
        COMMAND mcopy -i "${CMAKE_BINARY_DIR}/boot/floppy.img" "${CMAKE_BINARY_DIR}/boot/unifont.bfn" ::/UNIFONT.BFN
        COMMAND ${Python3_EXECUTABLE} "${CMAKE_SOURCE_DIR}/tools/injectbin/injectbin.py" "${CMAKE_CURRENT_BINARY_DIR}/stage1.bin" "${CMAKE_BINARY_DIR}/boot/floppy.img" 512
        DEPENDS bootloader_execbin stage1_bin
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Creating bootloader floppy image"
    )
endif()

if (BUILD_CDIMAGE)
    add_custom_target(bootloader_cdimage ALL
        COMMAND mkisofs -o "${CMAKE_BINARY_DIR}/boot/cdrom.iso" -b floppy.img -c boot.catalog "${CMAKE_BINARY_DIR}/boot/floppy.img"
        DEPENDS bootloader
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        COMMENT "Creating bootloader cd image"
    )
endif()

# add_custom_target(bootloader_hdimage ALL
#     COMMAND dd if=/dev/zero of="${CMAKE_BINARY_DIR}/boot/hd.img" bs=1048576 count=16
#     COMMAND "${CMAKE_BINARY_DIR}/tools/mkfs.afs/mkfs.afs" -b "${CMAKE_CURRENT_BINARY_DIR}/fdboot.bin" -j 2M -l "HDBOOT" -N 2 -O "emos" -r 8 -s 1 -u 00000000-0000-0000-0000-000000000000 -iv "${CMAKE_BINARY_DIR}/boot/hd.img"
#     DEPENDS bootloader mkfs.afs-host
#     WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
#     COMMENT "Creating bootloader hd image"
# )
