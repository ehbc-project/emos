    .org    0

    .set    STACK_START, 0x000A0000

    .section .text.startup
    .code32
    .globl  _start
_start:
    cld
    cli

    mov     $__bss_start, %edi          # clear bss
    mov     $__bss_end, %ecx
    sub     $__bss_start, %ecx
    add     $3, %ecx
    shr     $2, %ecx
    rep     stosl

    mov     $STACK_START, %esp
    mov     %esp, %ebp

    push    %edx

    mov     $__init_array_start, %ebx
0:
    cmp     $__init_array_end, %ebx
    je      1f
    call    *(%ebx)
    jmp     0b
1:

.jump:
    call    main                        # jump to c code.
                                        # this function should not return
    add     $4, %esp

.halt:
    hlt
    jmp     .halt
