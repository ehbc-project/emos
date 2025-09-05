    .org    0

    .section .text.startup
    .code32
    .globl  _start
_start:
    mov     $__bss_start, %edi
    mov     $__bss_end, %ecx
    sub     $__bss_start, %ecx
    rep     stosb

.jump:
    call    _pc_init                    # jump to c code.
                                        # this function should not return
