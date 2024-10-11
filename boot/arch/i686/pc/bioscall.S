
    .org    0
    .section .text
    .code32
    .globl  _pc_bios_call
_pc_bios_call:
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

    push    %ebp                        # store base pointer for later use

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
    movw    28(%edx), %ax
    movw    %ax, .Les
    movw    32(%edx), %ax
    movw    %ax, .Lds

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
.Lds:
    .word   0
    mov     %ax, %ds

    .byte   0xB8
.Lax:
    .word   0

    .byte   0xCD                        # int imm8
.Lirq:
    .byte   0

    mov     %ax, .Lax                   # store registers temporarily
    mov     %bx, .Lbx
    mov     %cx, .Lcx
    mov     %dx, .Ldx
    mov     %si, .Lsi
    mov     %di, .Ldi

    call    switch_real_to_prot         # switch back to protected mode
    .code32

    pop     %ebp                        # restore stack base pointer

    mov     12(%ebp), %edx              # load stuct pointer

    movw    .Lax, %ax                   # store registers
    movw    %ax, (%edx)
    movw    .Lbx, %ax
    movw    %ax, 4(%edx)
    movw    .Lcx, %ax
    movw    %ax, 8(%edx)
    movw    .Ldx, %ax
    movw    %ax, 12(%edx)
    movw    .Lsi, %ax
    movw    %ax, 16(%edx)
    movw    .Ldi, %ax
    movw    %ax, 20(%edx)


                                        # TODO: return carry flag value

    pop     %edi                        # pop registers
    pop     %esi
    pop     %ebx
    pop     %edx
    pop     %ecx
    pop     %eax

    pop     %ebp                        # restore base pointer

    ret
