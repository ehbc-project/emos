    .org    0

    .set    REAL_STACK_INIT, 0x00007BFC # right below the inital code area
    .set    PROT_STACK_INIT, 0x00003FFC

    .set    PROT_CODE_SEG, 0x08
    .set    PROT_DATA_SEG, 0x10
    .set    PROT16_CODE_SEG, 0x18
    .set    PROT16_DATA_SEG, 0x20

    .section .textlow
    .code16
    .globl switch_real_to_prot
switch_real_to_prot:
    cli                                 # clear interrupt

    xor     %ax, %ax
    mov     %ax, %ds                    # clear ds

    lgdt    gdtr                        # load gdt

    mov     %cr0, %eax                  # get cr0
    or      $1, %eax                    # enable protected mode
    mov     %eax, %cr0                  # set cr0

    jmp     $PROT_CODE_SEG, $.pmode     # switch code segment
                                        # the .code32 directive should be placed
                                        # after this line of code

    .code32
.pmode:
    mov     $PROT_DATA_SEG, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss                    # set segments

    xor     %eax, %eax
    mov     (%esp), %ax
    mov     %ax, REAL_STACK_INIT        # save current return address

    mov     prot_stack, %eax
    mov     %eax, %esp
    mov     %eax, %ebp                  # restore protected mode stack

    xor     %eax, %eax
    mov     REAL_STACK_INIT, %ax
    mov     %eax, (%esp)                # load current return address

    xor     %eax, %eax                  # clear eax

    ret

    .globl switch_prot_to_real
switch_prot_to_real:
    cli                                 # clear interrupt
    lgdt    gdtr                        # load gdt

    mov     %esp, %eax
    mov     %eax, prot_stack            # save stack

    mov     (%esp), %eax
    mov     %eax, REAL_STACK_INIT       # real mode stack is not preserved

    mov     $REAL_STACK_INIT, %eax
    mov     %eax, %esp
    mov     %eax, %ebp                  # set up stack pointer, base pointer

    ljmp    $PROT16_CODE_SEG, $.pmode16 # switch to 16-bit protected mode
                                        # the .code16 directive should be placed
                                        # after this line of code

    .code16
.pmode16:
    mov     $PROT16_DATA_SEG, %ax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss                    # set up segments

    mov     %cr0, %eax                  # get cr0
    and     $~1, %eax                   # disable protected mode
    mov     %eax, %cr0                  # set cr0

    ljmp    $0, $.rmode                 # reload code segment
                                        # assuming that we're in 0-FFFFh area

.rmode:                                 # real mode
    xor     %eax, %eax
    mov     %ax, %ds
    mov     %ax, %es
    mov     %ax, %fs
    mov     %ax, %gs
    mov     %ax, %ss                    # set up segments

    ret

    .section .data
    .align  8
    .word   0
prot_stack:                             # protected mode stack
    .long   PROT_STACK_INIT

gdtr:
    .word   8 * 5 - 1                   # 3 entries
    .long   gdt                         # gdt pointer
gdt:
    .word   0x0000                      # null descriptor
    .word   0x0000
    .byte   0x00
    .byte   0x00
    .byte   0x00
    .byte   0x00
    
    .word   0xFFFF                      # code segment
    .word   0x0000
    .byte   0x00
    .byte   0x9A
    .byte   0xCF
    .byte   0x00
        
    .word   0xFFFF                      # data segment
    .word   0x0000
    .byte   0x00
    .byte   0x92
    .byte   0xCF
    .byte   0x00
    
    .word   0xFFFF                      # 16-bit protected mode code segment
    .word   0x0000
    .byte   0x00
    .byte   0x9A
    .byte   0x00
    .byte   0x00

    .word   0xFFFF                      # 16-bit protected mode data segment
    .word   0x0000
    .byte   0x00
    .byte   0x92
    .byte   0x00
    .byte   0x00
