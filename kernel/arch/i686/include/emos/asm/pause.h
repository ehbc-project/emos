#ifndef __EMOS_ASM_ASM_PAUSE_H__
#define __EMOS_ASM_ASM_PAUSE_H__

#include <compiler.h>

__always_inline void _i686_pause(void)
{
    asm volatile ("pause");
}

#endif // __EMOS_ASM_ASM_PAUSE_H__
