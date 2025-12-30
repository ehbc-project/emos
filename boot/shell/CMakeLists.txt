cmake_minimum_required(VERSION 3.13)

add_subdirectory(commands)

target_sources(bootloader PRIVATE parse.c readline.c shell.c variable.c)
