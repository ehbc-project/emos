cmake_minimum_required(VERSION 3.13)

target_sources(bootloader PRIVATE bioscall.S disk.c keyboard.c mem.c misc.c video.c)
