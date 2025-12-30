# Gemini Code Assistant Context

## Project Overview

This project is **EMOS (EHBC Modular OS)**, a modular operating system. From the file structure and content, it appears to be a hobby OS project with a focus on modularity.

The project is written primarily in **C and Assembly**, and uses **CMake** for its build system. The target architecture appears to be **i686**.

The OS seems to have a bootloader, a kernel, and a set of userspace utilities and libraries. It supports various hardware components like PCI, USB, ACPI, and different file systems.

## Building and Running

### Building the Project

The project uses CMake to generate build files. To build the project, you would typically run:

```bash
cmake -S. -Bbuild -DCMAKE_BUILD_TYPE=Debug -DTARGET=i686-pc-bios
cmake --build build
scripts/mkdisk.sh -a i686 disk.img
```

This will create the necessary build files in the `build` directory and then compile the project. The output binaries, including the floppy and CD-ROM images, will be placed in the `build/boot` directory.

### Running the OS

The project includes a `run.sh` script to execute the OS in the QEMU emulator. To run the OS, you can execute this script:

```bash
scripts/run.sh pc-i386
```

This will start a QEMU virtual machine with the compiled OS images. The script configures QEMU with various virtual hardware devices.

## Development Conventions

*   **Directory Structure**: The project follows a well-defined directory structure, separating the bootloader, kernel, SDK, and user-level binaries. This is documented in the `README.md` file.
*   **Build System**: CMake is used as the build system. `CMakeLists.txt` files are present in most directories, defining the build process for each component.
*   **Coding Style**: The code appears to follow a consistent C coding style, with a focus on low-level system programming.
*   **Modularity**: The project is designed to be modular, with different components like drivers and file systems developed as separate modules.
