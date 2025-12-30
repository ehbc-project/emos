#ifndef __EBOOT_ASM_INTRINSICS_INVLPG_H__
#define __EBOOT_ASM_INTRINSICS_INVLPG_H__

#include <eboot/compiler.h>

__always_inline void _i686_invlpg(void *addr)
{
    asm volatile ("invlpg (%0)" : : "r"(addr) : "memory");
}

#endif // __EBOOT_ASM_INTRINSICS_INVLPG_H__
