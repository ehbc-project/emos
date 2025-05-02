.org    0
.section .text
.code32
.globl  _i686_idt_load
_i686_idt_load:
    push    %ebp                        # set up stack base pointer
    mov     %esp, %ebp

    push    %esi                        # push registers
    push    %edi

    mov     8(%ebp), %esi               # load IDT table address
    mov     12(%ebp), %di               # load IDT table size

    dec     %di                         # decrease IDT table size by 1

    xor     %eax, %eax                  # clear EAX

    push    %eax
    push    %esi
    push    %di

    lidt    (%esp)                      # load IDT

    pop     %edi                        # pop registers
    pop     %esi

    pop     %ebp                        # restore base pointer

    ret

