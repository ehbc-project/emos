OUTPUT_ARCH(i386)
OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")

ENTRY(_s1start)

SECTIONS
{
    .bss 0x00000500 :
    {
        PROVIDE_HIDDEN(__bss_start = .);
        *(.bss .bss.*)
        *(COMMON)
        PROVIDE_HIDDEN(__bss_end = .);
    }

    .text 0x00007E00 :
    {
        PROVIDE_HIDDEN(__text_start = .);
        KEEP(*(.text.startup .text.startup.*))
        KEEP(*(.text.low .text.low.*))
        KEEP(*(.text .text.*))
        PROVIDE_HIDDEN(__text_end = .);
    }

    .data :
    {
        PROVIDE_HIDDEN(__data_start = .);
        *(.data .data.*)
        PROVIDE_HIDDEN(__data_end = .);
    }

    .rodata :
    {
        PROVIDE_HIDDEN(__rodata_start = .);
        KEEP(*(.rodata .rodata.*))
        PROVIDE_HIDDEN(__rodata_end = .);
    }

    .init_array : ALIGN(4)
    {
        PROVIDE(__init_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array .ctors))
        PROVIDE(__init_array_end = .);
    }

    .fini_array : ALIGN(4)
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
        KEEP(*(.fini_array .dtors))
        PROVIDE_HIDDEN(__fini_array_end = .);
    }
    
    .init :
    {
        KEEP(*(SORT_NONE(.init)))
    }

    .fini :
    {
        KEEP(*(SORT_NONE(.fini)))
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

    PROVIDE_HIDDEN(__end = .);
}
