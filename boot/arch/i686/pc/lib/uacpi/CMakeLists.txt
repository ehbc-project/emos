cmake_minimum_required(VERSION 3.13)

target_sources(bootloader PRIVATE kernel_event.c kernel_interrupt.c kernel_io.c kernel_mem.c kernel_mutex.c kernel_other.c kernel_pci.c kernel_schedule.c kernel_spinlock.c libc.c)
