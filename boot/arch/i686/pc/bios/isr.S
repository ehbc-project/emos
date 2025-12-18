    .org    0

    .globl  _pc_isr_common

    .section .text
    .code32
    
    .macro  DEFINE_ISR num error
    .globl  _pc_isr_\num
_pc_isr_\num:
    cld

    pushal                              # push all registers
    pushw   %ds
    pushw   %es
    pushw   %ss
    pushw   $0

    push    %ebp                        # set up stack base pointer
    mov     %esp, %ebp

    mov     %esp, %eax
    add     $4, %eax

    .if     \error
    pushl   44(%ebp)                    # error num
    pushl   $0x\num                     # interrupt num
    pushl   %eax                        # registers

    lea     48(%ebp), %eax
    pushl   %eax                        # interrupt frame
    .else
    pushl   $0                          # error num
    pushl   $0x\num                     # interrupt num
    pushl   %eax                        # registers

    lea     44(%ebp), %eax
    pushl   %eax                        # interrupt frame
    .endif
    
    call    _pc_isr_common
    add     $16, %esp

    pop     %ebp                        # restore base pointer
    popw    %ax
    popw    %ss
    popw    %es
    popw    %ds
    popal                               # pop all registers

    iret                                # return from interrupt
    .endm

    DEFINE_ISR      00, 0
    DEFINE_ISR      01, 0
    DEFINE_ISR      02, 0
    DEFINE_ISR      03, 0
    DEFINE_ISR      04, 0
    DEFINE_ISR      05, 0
    DEFINE_ISR      06, 0
    DEFINE_ISR      07, 0
    DEFINE_ISR      08, 0
    DEFINE_ISR      09, 0
    DEFINE_ISR      0a, 1
    DEFINE_ISR      0b, 1
    DEFINE_ISR      0c, 1
    DEFINE_ISR      0d, 1
    DEFINE_ISR      0e, 1
    DEFINE_ISR      0f, 0
    DEFINE_ISR      10, 0
    DEFINE_ISR      11, 0
    DEFINE_ISR      12, 0
    DEFINE_ISR      13, 0
    DEFINE_ISR      14, 0
    DEFINE_ISR      15, 1
    DEFINE_ISR      16, 0
    DEFINE_ISR      17, 0
    DEFINE_ISR      18, 0
    DEFINE_ISR      19, 0
    DEFINE_ISR      1a, 0
    DEFINE_ISR      1b, 0
    DEFINE_ISR      1c, 0
    DEFINE_ISR      1d, 1
    DEFINE_ISR      1e, 1
    DEFINE_ISR      1f, 0

    .irpc   x,23456789abcdef
    .irpc   y,0123456789abcdef
    DEFINE_ISR      \x\y, 0
    .endr
    .endr
