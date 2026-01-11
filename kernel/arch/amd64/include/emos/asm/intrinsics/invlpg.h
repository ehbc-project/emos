#ifndef __EMOS_ASM_INTRINSICS_INVLPG_H__
#define __EMOS_ASM_INTRINSICS_INVLPG_H__

#include <emos/compiler.h>

__always_inline void _i686_invlpg(void *addr)
{
    asm volatile ("invlpg (%0)" : : "r"(addr) : "memory");
}

#endif // __EMOS_ASM_INTRINSICS_INVLPG_H__
