#!/bin/sh

i686-elf-as -g -c flopboot.S -o flopboot.o
i686-elf-as -g -c ../cpu_mode.S -o cpu_mode.o
i686-elf-as -g -c bioscall.S -o bioscall.o
i686-elf-gcc -g -c -ffreestanding main.c -o main.o
i686-elf-ld flopboot.o main.o cpu_mode.o bioscall.o -T ldscript.x -o boot.elf
i686-elf-objcopy -O binary boot.elf boot.bin
