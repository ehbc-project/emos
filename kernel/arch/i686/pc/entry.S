    .org    0

    .set    STACK_START, 0xC04FFFFF

    .section .text.startup
    .code32
    .globl  _start
_start:
    cld
    cli

    xor     %eax, %eax
    mov     $__bss_start, %edi          # clear bss
    mov     $__bss_end, %ecx
    sub     $__bss_start, %ecx
    add     $3, %ecx
    shr     $2, %ecx
    rep     stosl

    xor     %ebp, %ebp                  # reset stack chain
    mov     %edx, %ebx                  # edx: bootinfo struct pointer

    push    %ebx                        # call init
    call    _pc_init
    add     $4, %esp

    pushl   $64                         # allocate stack area
    pushl   $0x000C04C0
    call    mm_allocate_pages_to        # TODO: check return status
    add     $8, %esp

    mov     0xFFFFF000, %eax            # unmark page table of page directory #0
    shr     $12, %eax
    push    %eax
    push    %eax
    call    mm_pma_unmark_reserved      # TODO: check return status
    add     $8, %esp

    movl    $0, 0xFFFFF000              # unmap page directory #0

    mov     %cr3, %eax                  # flush tlb
    mov     %eax, %cr3

    mov     $STACK_START, %esp          # set new stack

    mov     $__init_array_start, %ebx   # run init array
0:
    cmp     $__init_array_end, %ebx
    je      1f
    call    *(%ebx)
    jmp     0b
1:

.jump:
    call    main                        # jump to c code.
                                        # this function should not return

.halt:
    hlt
    jmp     .halt
