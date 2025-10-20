    .org    0

    .set    STACK_START, 0x000A0000

    .section .text.startup
    .code32
    .globl  _start
_start:
    mov     $__bss_start, %edi
    mov     $__bss_end, %ecx
    sub     $__bss_start, %ecx
    shr     $2, %ecx
    rep     stosl

    mov     $STACK_START, %esp
    mov     %esp, %ebp

.jump:
    call    main                        # jump to c code.
                                        # this function should not return

.halt:
    hlt
    jmp     .halt
