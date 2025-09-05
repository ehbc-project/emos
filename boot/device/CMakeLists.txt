cmake_minimum_required(VERSION 3.13)

add_subdirectory(block)
add_subdirectory(hid)
add_subdirectory(video)
add_subdirectory(char)
add_subdirectory(usb_host)

target_sources(bootloader PRIVATE device.c driver.c resource.c)
