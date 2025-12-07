#ifndef __I686_ASM_HALT_H__
#define __I686_ASM_HALT_H__

#include <compiler.h>

__always_inline void _i686_halt()
{
    asm volatile ("hlt");
}

#endif // __I686_ASM_HALT_H__
