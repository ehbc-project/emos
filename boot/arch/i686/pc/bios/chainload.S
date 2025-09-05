    .org    0
    
    .section .text
    .code32
    .globl  _pc_chainload
_pc_chainload:
    push    %ebp
    mov     %esp, %ebp

    pushal
    cli

    mov     $0x60, %al              # enable translation
    outb    %al, $0x0064
    mov     $0x63, %al
    outb    %al, $0x0060

    pushl   $0x70                   # remap PIC
    pushl   $0x08
    call    _pc_remap_pic_int
    add     $8, %esp

    mov     8(%ebp), %dl            # get drive number

    call    switch_prot_to_real
    .code16
    xor     %ax, %ax
    mov     %ax, %es

    int     $0x13                   # reset disk 

    mov     $0x0201, %ax            # read the first sector
    mov     $0x0001, %cx
    mov     $0x00, %dh
    mov     $0x7C00, %bx
    int     $0x13

    jnc     _pc_chainload.continue
    call    switch_real_to_prot
    .code32

    pushl   $0x28                   # revert PIC
    pushl   $0x20
    call    _pc_remap_pic_int
    add     $8, %esp

    mov     $0x60, %al              # disable translation
    outb    %al, $0x0064
    mov     $0x03, %al
    outb    %al, $0x0060

    sti
    popal

    mov     $1, %eax
    leave   
    ret

    .code16
_pc_chainload.continue:
    xor     %ax, %ax                # Set video mode 0x03
    mov     $0x03, %al
    int     $0x10

    in      $0x92, %al              # Disable A20
    and     $0xFD, %al
    out     %al, $0x92

    xor     %eax, %eax              # Clear registers
    mov     %ax, %ds
    mov     %ax, %ss
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %eax, %ebx
    mov     %eax, %ecx
    mov     %eax, %esi
    mov     %eax, %edi
    mov     %eax, %ebp

    mov     $0x02, %ah
    sahf
    sti

    mov     $0x6F00, %esp
    mov     $0xAA55, %ax

    jmp     $0x0000, $0x7C00
