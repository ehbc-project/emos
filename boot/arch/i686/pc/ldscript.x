OUTPUT_ARCH(i386)
OUTPUT_FORMAT("elf32-i386", "elf32-i386", "elf32-i386")

ENTRY(_start)

SECTIONS
{
    .bss 0x00000500 : /* low */
    {
        PROVIDE_HIDDEN(__bss_low_start = .);
        *(.bss.low .bss.low.*)
        *(COMMON)
        PROVIDE_HIDDEN(__bss_low_end = .);
    }

    .text 0x00009000 : /* low */
    {
        PROVIDE_HIDDEN(__text_low_start = .);
        KEEP(*(.text.startup .text.startup.*))
        KEEP(*(.text.low .text.low.*))
        PROVIDE_HIDDEN(__text_low_end = .);
    }

    .data : /* low */
    {
        PROVIDE_HIDDEN(__data_low_start = .);
        *(.data.low .data.low.*)
        PROVIDE_HIDDEN(__data_low_end = .);
    }

    .rodata : /* low */
    {
        PROVIDE_HIDDEN(__rodata_low_start = .);
        KEEP(*(.rodata.low .rodata.low.*))
        PROVIDE_HIDDEN(__rodata_low_end = .);
    }

    .text :
    {
        PROVIDE_HIDDEN(__text_start = .);
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

    .bss 0x00100000 :
    {
        PROVIDE_HIDDEN(__bss_start = .);
        *(.bss .bss.*)
        PROVIDE_HIDDEN(__bss_end = .);
    }

    PROVIDE_HIDDEN(__end = .);
}
