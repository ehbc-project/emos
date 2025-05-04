
    .org    0
    .section .textlow
    .code32
    .globl  _pc_bios_call
_pc_bios_call:
    push    %ebp                        # set up stack base pointer
    mov     %esp, %ebp

    push    %ecx                        # push registers
    push    %edx
    push    %ebx
    push    %esi
    push    %edi

    mov     8(%ebp), %al                # arguments start from offset +8
    mov     %al, .Lirq                  # load irq number
    mov     12(%ebp), %edx              # load stuct pointer

    push    %ebp                        # store base pointer for later use

    mov     (%edx), %eax                 # load registers
    mov     %eax, .Lax
    mov     4(%edx), %eax
    mov     %eax, .Lbx
    mov     8(%edx), %eax
    mov     %eax, .Lcx
    mov     12(%edx), %eax
    mov     %eax, .Ldx
    mov     16(%edx), %eax
    mov     %eax, .Lsi
    mov     20(%edx), %eax
    mov     %eax, .Ldi
    mov     24(%edx), %ax
    mov     %ax, .Lds
    mov     26(%edx), %ax
    mov     %ax, .Les

    call    switch_prot_to_real         # switch to real mode
    .code16
    sti

    .byte   0x66, 0xB8                  # mov imm32, %eax
.Lbx:
    .long   0
    mov     %eax, %ebx                  # move to proper register

    .byte   0x66, 0xB8                  # mov imm32, %eax
.Lcx:
    .long   0
    mov     %eax, %ecx

    .byte   0x66, 0xB8                  # mov imm32, %eax
.Ldx:
    .long   0
    mov     %eax, %edx

    .byte   0x66, 0xB8                  # mov imm32, %eax
.Lsi:
    .long   0
    mov     %eax, %esi

    .byte   0x66, 0xB8                  # mov imm32, %eax
.Ldi:
    .long   0
    mov     %eax, %edi

    .byte   0xB8                        # mov imm16, %ax
.Les:
    .word   0
    mov     %ax, %es

    .byte   0xB8                        # mov imm16, %ax
.Lds:
    .word   0
    mov     %ax, %ds

    .byte   0x66, 0xB8                  # mov imm32, %eax
.Lax:
    .long   0

    .byte   0xCD                        # int imm8
.Lirq:
.Lerr:
    .byte   0

    mov     %eax, .Lax                   # store registers temporarily
    mov     %ebx, .Lbx
    mov     %ecx, .Lcx
    mov     %edx, .Ldx
    mov     %esi, .Lsi
    mov     %edi, .Ldi

    xor     %al, %al
    setc    %al
    mov     %al, .Lerr                  # store carry flag

    call    switch_real_to_prot         # switch back to protected mode
    .code32

    pop     %ebp                        # restore stack base pointer

    mov     12(%ebp), %edx              # load stuct pointer

    mov     .Lax, %eax                   # store registers
    mov     %eax, (%edx)
    mov     .Lbx, %eax
    mov     %eax, 4(%edx)
    mov     .Lcx, %eax
    mov     %eax, 8(%edx)
    mov     .Ldx, %eax
    mov     %eax, 12(%edx)
    mov     .Lsi, %eax
    mov     %eax, 16(%edx)
    mov     .Ldi, %eax
    mov     %eax, 20(%edx)

    xor     %eax, %eax
    movb    .Lerr, %al

    pop     %edi                        # pop registers
    pop     %esi
    pop     %ebx
    pop     %edx
    pop     %ecx

    pop     %ebp                        # restore base pointer

    ret
