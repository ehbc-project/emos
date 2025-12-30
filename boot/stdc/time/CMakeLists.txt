cmake_minimum_required(VERSION 3.13)

target_sources(bootloader PRIVATE gmtime.c localtime.c mktime.c time.c)
