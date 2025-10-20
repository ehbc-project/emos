    .org    0
    
    .section .text
    .code32
    .globl  _i686_breakpoint
_i686_breakpoint:
    int3

    ret
