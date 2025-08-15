cmake_minimum_required(VERSION 3.13)

add_subdirectory(pci)
add_subdirectory(usb)

target_sources(bootloader PRIVATE bus.c)
