    .org    0
    
    .section .text
    .code32
    .globl  _i686_pc_halt
_i686_pc_halt:
    cli
.halt:
    hlt
    jmp     .halt
