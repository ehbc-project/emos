cmake_minimum_required(VERSION 3.13)

add_subdirectory(disk)
add_subdirectory(hid)
add_subdirectory(video)
add_subdirectory(serial)
add_subdirectory(usb_host)

target_sources(bootloader PRIVATE device.c driver.c resource.c)
