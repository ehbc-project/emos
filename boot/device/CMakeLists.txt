cmake_minimum_required(VERSION 3.13)

add_subdirectory(hid)
add_subdirectory(video)
add_subdirectory(virtual)

target_sources(bootloader PRIVATE device.c driver.c)
