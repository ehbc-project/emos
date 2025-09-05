    .org    0
    
    .section .text
    .code32
    .globl  _i686_breakpoint
_i686_breakpoint:
    push    %ebp                        # set up stack base pointer
    mov     %esp, %ebp

    int3

    pop     %ebp                        # restore base pointer

    ret
