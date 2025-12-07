cmake_minimum_required(VERSION 3.13)

add_subdirectory(afs)
add_subdirectory(fat)
add_subdirectory(iso9660)

target_sources(bootloader PRIVATE fs.c)
