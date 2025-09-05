cmake_minimum_required(VERSION 3.13)

target_sources(bootloader PRIVATE i8042.c ps2_keyboard.c ps2_mouse.c)
