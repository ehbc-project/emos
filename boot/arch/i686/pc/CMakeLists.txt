cmake_minimum_required(VERSION 3.13)

target_include_directories(bootloader PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_sources(bootloader PRIVATE bios/bioscall.S bootinfo.c bios/disk.c flopboot.S init.c bios/keyboard.c bios/video.c bios/mem.c pci/cfgspace.c font.c)
target_link_options(bootloader PUBLIC -T "${CMAKE_CURRENT_SOURCE_DIR}/ldscript.x")
set_target_properties(bootloader PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ldscript.x")
