# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

EMOS (EHBC Modular OS) is a hobby/educational operating system targeting x86 (i686) architecture with a clean, modular design. The project consists of a multi-stage boot process (MBR/VBR → Bootloader → Kernel) with the bootloader being the most developed component.

## Build System

**Prerequisites:**
- CMake 3.13+
- i686-elf cross-compilation toolchain (gcc, g++, binutils)
- QEMU for testing

**Key Build Commands:**
```bash
# Configure and build
mkdir build && cd build
cmake .. -DTARGET_ARCH=i686 -DTARGET_BOARD=pc
make

# Run in QEMU
./run.sh  # Uses qemu-system-i386 with floppy and hard disk images
```

**Important Build Files:**
- `CMakeLists.txt` - Main build configuration
- `cmake/I686ElfToolchain.cmake` - Cross-compilation toolchain configuration
- `boot/CMakeLists.txt` - Bootloader build configuration

## Architecture

### Boot Process Flow
1. **BIOS** loads MBR/VBR from floppy (`flopboot.S`) or hard disk
2. **VBR** loads the bootloader executable
3. **Bootloader** (`boot/`) initializes hardware, loads kernel
4. **Kernel** (`kernel/`) takes control (early development stage)

### Key Components

**Bootloader (`/boot/`):**
- `arch/i686/pc/` - x86 PC platform-specific code (BIOS, ACPI, PCI)
- `core/` - Main bootloader logic (`main.c`)
- `device/` - Device drivers with interface-based design
  - `video/` - VBE graphics, virtual console
  - `virtual/` - ANSI terminal emulation
- `fs/` - File system support (FAT, ISO9660)
- `bus/` - PCI and USB host controllers (UHCI, EHCI, xHCI)
- `mm/` - Memory management
- `stdc/` - Custom standard C library implementation

**Device Driver Model:**
- Interface-based design (`interface/` headers)
- Block devices, character devices, console, framebuffer interfaces
- Driver registration system (`device/driver.h`)

### Important Code Patterns

1. **Hardware Abstraction:**
   - Architecture-specific code in `arch/{arch}/`
   - Platform-specific code in `arch/{arch}/{platform}/`
   - Generic interfaces in `include/interface/`

2. **Memory Management:**
   - Custom allocator in `mm/mm.c`
   - Boot-time memory operations

3. **Standard Library:**
   - Custom implementations in `stdc/`
   - Freestanding environment (no host libc)

## Development Guidelines

1. **Code Style:**
   - C11 for C code, C++14 for C++ code
   - AT&T syntax for assembly
   - Strict error checking (-Werror -Wall)

2. **Adding New Features:**
   - Follow existing directory structure
   - Implement proper interfaces for device drivers
   - Update relevant CMakeLists.txt files

3. **Testing:**
   - Use `run.sh` for QEMU testing
   - Floppy boot image: `build/boot/arch/i686/pc/floppy.img`
   - Debug with QEMU's `-s` flag (GDB stub on port 1234)

## Current State

- **Bootloader**: Functional with VBE graphics, ANSI terminal, FAT/ISO9660 support
- **Kernel**: Early development stage
- **Recent Work**: VBE graphics, ANSI terminal emulation, GNU Unifont rendering, FAT filesystem

## Key Files to Understand

- `boot/core/main.c` - Bootloader entry point and main logic
- `boot/arch/i686/pc/init.c` - Platform initialization
- `boot/arch/i686/pc/ldscript.lds` - Linker script for bootloader layout
- `flowchart.md` - Detailed boot process and architecture documentation