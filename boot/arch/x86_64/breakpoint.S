    .org    0
    
    .section .text
    .code64
    .globl  _x86_64_breakpoint
_x86_64_breakpoint:
    int3

    ret
