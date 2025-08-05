    .org    0
    
    .section .text.low
    .code32
    .globl  _pc_bios_chainload
_pc_bios_chainload:
    push    %ebp
    mov     %esp, %ebp

    mov     8(%ebp), %dl

    call    switch_prot_to_real
    .code16
    xor     %ax, %ax                # Read the first sector
    mov     %ax, %es
    mov     $0x0201, %ax
    mov     $0x0001, %cx
    mov     $0x00, %dh
    mov     $0x7C00, %bx
    int     $0x13

    jnc     _pc_bios_chainload.continue
    call    switch_real_to_prot
    .code32

    mov     $1, %eax
    pop     %ebp
    ret

    .code16
_pc_bios_chainload.continue:
    xor     %ax, %ax                # Set video mode 0x03
    mov     $0x03, %al
    int     $0x10

    in      $0x92, %al              # Disable A20
    and     $0xFD, %al
    out     %al, $0x92

    xor     %ax, %ax                # Clear registers
    mov     %ax, %ds
    mov     %ax, %ss
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %bx
    mov     %ax, %cx
    mov     %ax, %si
    mov     %ax, %di
    mov     %ax, %sp
    mov     %ax, %bp

    jmp     $0x0000, $0x7C00
