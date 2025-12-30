# EHBC Modular OS (EMOS) Repository

## Build

```sh
git clone https://github.com/ehbc-project/emos
cd emos
cmake -S. -Bbuild -DTARGET=i686-pc-bios
cmake --build build
scripts/mkdisk.sh -a i686 disk.img
scripts/run.sh pc-i386
```

## Directory Structure

- `docs`: Documents
- `dist`: Scripts for creating disk images of system or installer
- `cmake`: CMake scripts
- `config`: Build configuration presets for each architecture
- `boot`: Bootloaders for each architecture / platform
- `bin`: System utility executables (e.g. `ls`, `sh`, `tar`, ...)
- `kernel`: Kernel sources
  - `arch`: Architecture- or machine-dependent codes
  - `drivers`: Device drivers
  - `exec`: Codes for loading and executing binaries
  - `fs`: File system drivers
    - `partition`: Partition table drivers
  - `init`: Kernel initialization codes
  - `ipc`: Inter-process communication codes
  - `mm`: Codes for system memory management
  - `posix`: POSIX translation layer
  - `subsys`: Kernel subsystems
    - `gui`: GUI subsystem
  - `syscall`: Syscall handlers
- `packages`: System packages
  - `emossdk`: Software Development Kit for EMOS
    - `libemos`: Common system call wrapper library
      - `conmanip`: Text console manipulation
      - `exec`: Process, thread, and dynamic library controls
      - `io`: I/O control functions
      - `memory`: Memory allocation and mapping functions
      - `ipc`: Inter-process communication functions
    - `libemosgui`: Graphical UI / drawing library
    - `libemosimm`: Library for Input Method Module (IMM)
    - `libemosutil`: OS-independent utility functions library
      - `crypto`: Simple cryptography
      - `unicode`: Unicode string manipulation
      - `compress`: Data compression / decompression
    - `libstdc`: C Standard library
    - `libstdcxx`: C++ Standard library
