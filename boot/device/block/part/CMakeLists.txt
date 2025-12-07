cmake_minimum_required(VERSION 3.13)

target_sources(bootloader PRIVATE gpt.c mbr.c part.c)
