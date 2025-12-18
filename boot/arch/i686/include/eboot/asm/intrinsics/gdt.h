#ifndef __EBOOT_ASM_INTRINSICS_GDT_H__
#define __EBOOT_ASM_INTRINSICS_GDT_H__

#include <stdint.h>

#include <eboot/asm/gdt.h>

#include <eboot/compiler.h>

__always_inline void _i686_lgdt(struct gdtr *gdtr)
{
    asm volatile ("lgdt (%0)" : : "r"(gdtr));
}


#endif // __EBOOT_ASM_INTRINSICS_GDT_H__
