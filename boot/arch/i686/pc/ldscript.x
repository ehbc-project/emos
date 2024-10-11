OUTPUT_ARCH(i386)
OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")

ENTRY(_start)

SECTIONS
{
    .bss 0x0000800 :
    {
        PROVIDE_HIDDEN(__bss_start = .);
        *(.bss .bss.*)
        *(COMMON)
        PROVIDE_HIDDEN(__bss_end = .);
    }

    .text 0x00007C00 :
    {
        KEEP(*(.text.startup .text.startup.*))
        KEEP(*(.text .text.*))
    }

    .data :
    {
        *(.data .data.*)
    }

    .init :
    {
        KEEP(*(SORT_NONE(.init)))
    }

    .fini :
    {
        KEEP(*(SORT_NONE(.fini)))
    }

    . = ALIGN(4);
    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array .ctors))
        PROVIDE_HIDDEN(__init_array_end = .);
    }

    . = ALIGN(4);
    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
        KEEP(*(.fini_array .dtors))
        PROVIDE_HIDDEN(__fini_array_end = .);
    }

    .ctors :
    {
        KEEP(*(SORT(.ctors.*)))
        KEEP(*(.ctors))
    }

    .dtors :
    {
        KEEP(*(SORT(.dtors.*)))
        KEEP(*(.dtors))
    }

    .rodata :
    {
        KEEP(*(.rodata .rodata.*))
    }
}
