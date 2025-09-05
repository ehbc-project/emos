    .org    0
    
    .section .text
    .code32
    .globl  _i686_out8
_i686_out8:
    push    %ebp                        # set up stack base pointer
    mov     %esp, %ebp

    mov     8(%ebp), %dx                # port
    mov     12(%ebp), %al               # value

    outb    %al, %dx                    # do the output

    pop     %ebp                        # restore base pointer

    ret

    .globl  _i686_out16
_i686_out16:
    push    %ebp                        # set up stack base pointer 
    mov     %esp, %ebp

    mov     8(%ebp), %dx                # port
    mov     12(%ebp), %ax               # value

    outw    %ax, %dx                    # do the output

    pop     %ebp                        # restore base pointer

    ret

    .globl  _i686_out32
_i686_out32:
    push    %ebp                        # set up stack base pointer
    mov     %esp, %ebp

    mov     8(%ebp), %dx                # port
    mov     12(%ebp), %eax              # value

    outl    %eax, %dx                   # do the output

    pop     %ebp                        # restore base pointer

    ret

    .globl  _i686_in8
_i686_in8:
    push    %ebp                        # set up stack base pointer
    mov     %esp, %ebp

    mov     8(%ebp), %dx                # port

    inb     %dx, %al                    # do the input

    pop     %ebp                        # restore base pointer

    ret

    .globl  _i686_in16
_i686_in16:
    push    %ebp                        # set up stack base pointer
    mov     %esp, %ebp

    mov     8(%ebp), %dx                # port

    inw     %dx, %ax                    # do the input

    pop     %ebp                        # restore base pointer

    ret

    .globl  _i686_in32
_i686_in32:
    push    %ebp                        # set up stack base pointer
    mov     %esp, %ebp

    mov     8(%ebp), %dx                # port

    inl     %dx, %eax                   # do the input

    pop     %ebp                        # restore base pointer

    ret
