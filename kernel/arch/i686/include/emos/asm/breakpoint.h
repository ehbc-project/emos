#ifndef __I686_BREAKPOINT_H__
#define __I686_BREAKPOINT_H__

#include <compiler.h>

__always_inline void _i686_breakpoint(void)
{
    asm volatile ("int3");
}

#endif // __I686_BREAKPOINT_H__
