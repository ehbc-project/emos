cmake_minimum_required(VERSION 3.13)

target_sources(bootloader PRIVATE bioscall.S bootinfo.c disk.c flopboot.S init.c keyboard.c video.c)
target_link_options(bootloader PUBLIC -T "${CMAKE_CURRENT_SOURCE_DIR}/ldscript.x")
set_target_properties(bootloader PROPERTIES LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ldscript.x")
