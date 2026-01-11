#ifndef __EMOS_ASM_INTRINSICS_IDT_H__
#define __EMOS_ASM_INTRINSICS_IDT_H__

#include <stdint.h>

#include <emos/asm/idt.h>

#include <emos/compiler.h>

__always_inline void _i686_lidt(struct idtr *idtr)
{
    asm volatile ("lidt (%0)" : : "r"(idtr));
}


#endif // __EMOS_ASM_INTRINSICS_IDT_H__
