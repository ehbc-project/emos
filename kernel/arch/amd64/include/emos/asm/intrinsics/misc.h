#ifndef __EMOS_ASM_INTRINSICS_MISC_H__
#define __EMOS_ASM_INTRINSICS_MISC_H__

#include <cpuid.h>

#include <stdint.h>

#include <emos/compiler.h>

__always_inline void _i686_interrupt_enable(void)
{
    asm volatile ("sti");
}

__always_inline void _i686_interrupt_disable(void)
{
    asm volatile ("cli");
}

__always_inline void _i686_halt(void)
{
    asm volatile ("hlt");
}

__always_inline void _i686_pause(void)
{
    asm volatile ("pause");
}

#endif // __EMOS_ASM_INTRINSICS_MISC_H__
