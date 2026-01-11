#ifndef __EMOS_ASM_INTRINSICS_RDTSC_H__
#define __EMOS_ASM_INTRINSICS_RDTSC_H__

#include <stdint.h>

#include <emos/compiler.h>

__always_inline uint64_t _i686_rdtsc(void)
{
    uint32_t low, high;
    asm volatile ("rdtsc" : "=a"(low), "=d"(high));
    return ((uint64_t)high << 32) | low;
}


#endif // __EMOS_ASM_INTRINSICS_RDTSC_H__
