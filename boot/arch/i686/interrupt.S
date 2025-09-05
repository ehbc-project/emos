    .org    0
    
    .section .text
    .code32
    .globl  _i686_enable_interrupt
_i686_enable_interrupt:
    sti

    ret

    .globl  _i686_disable_interrupt
_i686_disable_interrupt:
    cli

    ret
