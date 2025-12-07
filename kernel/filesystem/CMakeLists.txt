cmake_minimum_required(VERSION 3.13)

add_subdirectory(afs)
add_subdirectory(fat)
add_subdirectory(iso9600)

target_sources(kernel PRIVATE filesystem.c)
