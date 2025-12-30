
# EHBC-OS Process Flowchart

## Bootstrapping Sequence Stage 1 (Bootloader Stage)

EHBC-FW                                 | Bootloader
----------------------------------------|-------------------------------
Detect and configure memory settings    | -
Generate memory map table               | -
Detect and initialize attached devices  | -
Generate device table                   | -
Find storage devices                    | -
Initialize pre-init drivers             | -
Initialize pre-init services            | -
Initialize pre-init service table       | -
Find bootloader executable from storage | -
Verify and load bootloader executable   | -
Jump to the loaded bootloader           | -
\-                                      | Request generated tables
Return tables to the bootloader         | -
\-                                      | Read configuration file from boot partition
Copy file contents to bootloader        | -
\-                                      | Parse configuration file
\-                                      | Find, verify, and load kernel
\-                                      | Verify and preload essential kernel-mode drivers
\-                                      | Request firmware deinitialization
Deinit and unload pre-init services     | -
Deinit and unload pre-init drivers      | -
\-                                      | Jump to the kernel

## Bootstrapping Sequence Stage 2 (Kernel Stage)

- Cleanup firmware and bootloader memory region
- Detect and initialize architecture-specific features
- Detect additional devices and peripherals
- Generate system information table
- Initialize preloaded kernel-mode drivers
- Read kernel configuration file
- Find, verify and load additional kernel-mode drivers
- Initialize additional kernel-mode drivers
- Generate kernel driver table
- Configure and initialize MMU
- Load and initialize Shared Object Manager (SOM) Service
- Load and initialize kernel services
- Execute initial shell

## System Virtual File System (VFS) Structure

### File Path Format

- Identifier format: `[^\x0-\x1F\x7F:;\/]+`
- File path format: `({Identifier}:)?\/?({Identifier}\/)*({Identifier})?`

**Storage Type Identifiers**

- `hd`: Hard Disk Drive / Solid State Drive
- `fd`: Floppy Disk
- `usb`: USB Flash Memory
- `cdrom`: CD/DVD/Blu-Ray Disc
- `net`: Network Storage (ex: NFS, SMB, FTP)
- `vd`: Virtual Storage (RAM Drive, Loop device)

**Reserved Drive Names**

- `sys`: Alias for the drive that contains the system kernel files
- `boot`: Alias for the drive the system boots from
- `dev`: Namespace of the registered system resources, similar to `/dev` in UNIX-like systems
- `unix`: An namespace emulating an UNIX root filesystem
- `kernel`: A read-only filesystem storing the kernel image


## Process Execution Sequence

Kernel                                  | Process
----------------------------------------|-----------------------------
Load executables from storage device    | -
Lookup shared object table              | -
Load unloaded shared object             | -
Relocate memory region                  | -
Copy command-line options               | -
Modify MMU descriptor tables            | -
Generate process descriptor table       | -
Register process to the task buffer     | -
...                                     | ...
\-                                      | Run user-level code
...                                     | ...
\-                                      | Register a function to the Event Vector Table (EVT)
(When if a signal or an event caused)   | -
Read the EVT and jump to the function   | -
\-                                      | Run event handler
...                                     | ...
\-                                      | Request system call
Process system call and return          | -
...                                     | ...
\-                                      | Request shared object
Load if the reqested object is unloaded | -
Relocate the memory map                 | -
Return the symbol table of the object   | -
...                                     | ...
\-                                      | Return from the process entry point
Release requested system resources      | -
Cleanup memory regions of the process   | -

## 
