    .org    0
    
    .section .text
    .code64
    .globl  _x86_64_enable_interrupt
_x86_64_enable_interrupt:
    sti

    ret

    .globl  _x86_64_disable_interrupt
_x86_64_disable_interrupt:
    cli

    ret
