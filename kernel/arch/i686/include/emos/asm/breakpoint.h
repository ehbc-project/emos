#ifndef __EMOS_ASM_BREAKPOINT_H__
#define __EMOS_ASM_BREAKPOINT_H__

#include <compiler.h>

__always_inline void _i686_breakpoint(void)
{
    asm volatile ("int3");
}

#endif // __EMOS_ASM_BREAKPOINT_H__
