#ifndef __EBOOT_ASM_INTRINSICS_RDTSC_H__
#define __EBOOT_ASM_INTRINSICS_RDTSC_H__

#include <stdint.h>

#include <eboot/compiler.h>

__always_inline uint64_t _i686_rdtsc(void)
{
    uint32_t low, high;
    asm volatile ("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}


#endif // __EBOOT_ASM_INTRINSICS_RDTSC_H__
