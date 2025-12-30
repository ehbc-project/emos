#ifndef __EMOS_ASM_ASM_HALT_H__
#define __EMOS_ASM_ASM_HALT_H__

#include <compiler.h>

__always_inline void _i686_halt()
{
    asm volatile ("hlt");
}

#endif // __EMOS_ASM_ASM_HALT_H__
