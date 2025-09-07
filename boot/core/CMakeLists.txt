cmake_minimum_required(VERSION 3.13)

target_sources(bootloader PRIVATE main.c panic.c getopt.c path.c crc32.c)
