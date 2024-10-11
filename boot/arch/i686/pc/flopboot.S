
    .set    MAGIC, 0xAA55

    .org    0
    .section .text
    .code16
    .globl  _start
_start:
    jmp     start1

start1:
    xor     %ax, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %ss                    # clear segment registers
    mov     $0x7C00, %sp                # setup stack pointer to $0000h:$7C00h

.read_disk:
    mov     $0x02, %ah                  # read
    mov     $0x11, %al                  # 17 sectors
    mov     $0x00, %ch                  # starting from cylinder 0
    mov     $0x02, %cl                  # sector 2
    mov     $0x00, %dh                  # head 0
                                        # from the drive booted from
    mov     $0x7E00, %bx                # to buffer starts after this code
    int     $0x13                       # do it!

    jnc     start2                      # jump to next step if there's no error

.print_error:
    mov     $0x0E, %ah                  # else, print
    mov     $str_diskerror, %si         # error string
    xor     %bx, %bx                    # to page 0

.print_error.1:
    lodsb
    or      %al, %al
    jz      .print_error.2              # until the char is NULL

    int     $0x10                       # do it!
    jmp     .print_error.1              # print next character

.print_error.2:
    jmp     .read_disk                  # retry

str_diskerror: 
    .asciz  "Error reading disk. Retrying...\r\n"

    .org    0x1FE                       # 512 - 2
    .word   MAGIC                       # magic number

    .org    0x200                       # now there's 17 more sectors for code!
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

start2:
    mov     %dl, _pc_boot_drive         # store the index of the boot drive

.activate_a20:
    mov     $0x2401, %ax                # activate A20 via bios
    int     $0x15
    jnc     start2.1                    # finish if there's no error

    mov     $str_a20error, %si
    call    print_string                # else print error
.activate_a20.halt:
    hlt                                 # and halt forever
    jmp     .activate_a20.halt


start2.1:
    call    switch_real_to_prot         # switch to protected mode
    .code32
    call    _pc_init                    # jump to c code.
                                        # this function should not return

.halt:                                  # if the function returns,
    hlt                                 # halt forever
    jmp     .halt

str_a20error:
    .asciz "Error activating A20 line.\r\n"
