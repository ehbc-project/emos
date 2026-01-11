#ifndef __EMOS_ASM_INTRINSICS_GDT_H__
#define __EMOS_ASM_INTRINSICS_GDT_H__

#include <stdint.h>

#include <emos/asm/gdt.h>

#include <emos/compiler.h>

__always_inline void _i686_lgdt(struct gdtr *gdtr)
{
    asm volatile ("lgdt (%0)" : : "r"(gdtr));
}


#endif // __EMOS_ASM_INTRINSICS_GDT_H__
