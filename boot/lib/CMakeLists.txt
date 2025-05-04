cmake_minimum_required(VERSION 3.13)

include(uacpi/uacpi.cmake)
# target_sources(bootloader PRIVATE ${UACPI_SOURCES})
target_include_directories(bootloader PRIVATE ${UACPI_INCLUDES})
