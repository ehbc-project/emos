#ifndef __EMOS_ASM_INTRINSICS_LTR_H__
#define __EMOS_ASM_INTRINSICS_LTR_H__

#include <stdint.h>

#include <emos/compiler.h>

__always_inline void _i686_ltr(uint16_t sel)
{
    asm volatile ("ltr %%ax" : : "a"(sel));
}

#endif // __EMOS_ASM_INTRINSICS_LTR_H__
