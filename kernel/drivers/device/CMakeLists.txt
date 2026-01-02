cmake_minimum_required(VERSION 3.13)

add_subdirectory(block)
add_subdirectory(char)
add_subdirectory(hid)
add_subdirectory(net)
add_subdirectory(rtc)
add_subdirectory(scsi)
add_subdirectory(usb)
add_subdirectory(video)

target_sources(kernel PRIVATE device.c)
