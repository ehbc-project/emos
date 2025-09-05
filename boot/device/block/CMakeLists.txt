cmake_minimum_required(VERSION 3.13)

add_subdirectory(part)

target_sources(bootloader PRIVATE ide_isa.c ata.c atapi.c fdc_isa.c floppy.c)
