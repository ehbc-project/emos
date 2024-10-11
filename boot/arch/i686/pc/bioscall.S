
    .org    0
    .section .text
    .code32
    .globl  bios_call
bios_call:
    push    %ebp                        # set up stack base pointer
    mov     %esp, %ebp

    push    %eax                        # push registers
    push    %ecx
    push    %edx
    push    %ebx
    push    %esi
    push    %edi

    movb    8(%ebp), %al                # arguments start from offset +8
    movb    %al, .Lirq                  # load irq number
    mov     12(%ebp), %edx              # load stuct pointer

    movw    (%edx), %ax                 # load registers
    movw    %ax, .Lax
    movw    4(%edx), %ax
    movw    %ax, .Lbx
    movw    8(%edx), %ax
    movw    %ax, .Lcx
    movw    12(%edx), %ax
    movw    %ax, .Ldx
    movw    16(%edx), %ax
    movw    %ax, .Lsi
    movw    20(%edx), %ax
    movw    %ax, .Ldi
    movw    24(%edx), %ax
    movw    %ax, .Les

    call    switch_prot_to_real         # switch to real mode
    .code16
    sti

    .byte   0xB8                        # mov imm16, %ax
.Lbx:
    .word   0
    mov     %ax, %bx                    # move to proper register

    .byte   0xB8
.Lcx:
    .word   0
    mov     %ax, %cx

    .byte   0xB8
.Ldx:
    .word   0
    mov     %ax, %dx

    .byte   0xB8
.Lsi:
    .word   0
    mov     %ax, %si

    .byte   0xB8
.Ldi:
    .word   0
    mov     %ax, %di

    .byte   0xB8
.Les:
    .word   0
    mov     %ax, %es

    .byte   0xB8
.Lax:
    .word   0

    .byte   0xCD                        # int imm8
.Lirq:
    .byte   0

    mov     8(%ebp), %edi               # edi may not be used to store data

    mov     %ax, (%edi)                 # store registers
    mov     %bx, 4(%edi)
    mov     %cx, 8(%edi)
    mov     %dx, 12(%edi)
    mov     %si, 16(%edi)
    mov     %di, 20(%edi)
    mov     %es, 24(%edi)

    call    switch_real_to_prot         # switch back to protected mode
    .code32

                                        # TODO: return carry flag value

    pop     %edi                        # pop registers
    pop     %esi
    pop     %ebx
    pop     %edx
    pop     %ecx
    pop     %eax

    pop     %ebp                        # restore base pointer

    ret
