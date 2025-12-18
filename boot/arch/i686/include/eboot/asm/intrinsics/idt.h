#ifndef __EBOOT_ASM_INTRINSICS_IDT_H__
#define __EBOOT_ASM_INTRINSICS_IDT_H__

#include <stdint.h>

#include <eboot/asm/idt.h>

#include <eboot/compiler.h>

__always_inline void _i686_lidt(struct idtr *idtr)
{
    asm volatile ("lidt (%0)" : : "r"(idtr));
}


#endif // __EBOOT_ASM_INTRINSICS_IDT_H__
