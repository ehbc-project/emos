    .org    0
    .section .text.startup
    .code16
    .globl  _start
_start:
    xor     %ax, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %ss

    mov     $0x9000, %sp
    mov     %sp, %bp

    mov     %dl, _pc_boot_drive         # store the index of the boot drive

.activate_a20:
    mov     $0x2401, %ax                # activate A20 via bios
    int     $0x15
    jnc     _start.1                    # finish if there's no error

    mov     $str_a20error, %si
    call    print_string                # else print error
    
    jmp     .halt

_start.1:
    call    switch_real_to_prot         # switch to protected mode
    .code32
    call    _pc_init                    # jump to c code.
                                        # this function should not return

.halt:                                  # if the function returns,
    hlt                                 # halt forever
    jmp     .halt

print_string:
    mov     $0x0E, %ah                  # print the string in %si
    xor     %bx, %bx

print_string.1:
    lodsb
    or      %al, %al
    jz      print_string.2              # until the char is NULL

    int     $0x10                       # do it!
    jmp     print_string.1              # print next character

print_string.2:
    ret                                 # return

str_a20error:
    .asciz "Error activating A20 line.\r\n"
