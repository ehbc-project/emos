    .org    0
    
    .section .text
    .code64
    .globl  _x86_64_pause
_x86_64_pause:
    pause

    ret
