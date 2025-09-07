OUTPUT_ARCH(i386)
OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")

ENTRY(_start)

SECTIONS
{
    .bss 0x00000500 :
    {
        PROVIDE_HIDDEN(__bss_start = .);
        *(.bss .bss.*)
        *(COMMON)
        PROVIDE_HIDDEN(__bss_end = .);
    }

    .text 0x00007C00 :
    {
        KEEP(*(.text .text.*))
    }

    .data :
    {
        *(.data .data.*)
    }
    
    .rodata :
    {
        KEEP(*(.rodata .rodata.*))
    }
}
