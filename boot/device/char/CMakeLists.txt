cmake_minimum_required(VERSION 3.13)

target_sources(bootloader PRIVATE ansiterm.c debugout.c ieee1284_isa.c uart_isa.c)
