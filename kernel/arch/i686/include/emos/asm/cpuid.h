#ifndef __EMOS_ASM_CPUID_H__
#define __EMOS_ASM_CPUID_H__

#include <cpuid.h>

#include <stdint.h>

#include <compiler.h>

enum cpuid_request {
    CPUID_GET_VENDOR_STRING = 0,
    CPUID_GET_FEATURES,
    CPUID_GET_TLB,
    CPUID_GET_SERIAL,

    CPUID_INTEL_EXTENDED = 0x80000000,
    CPUID_INTEL_FEATURES,
    CPUID_INTEL_BRAND_STRING,
    CPUID_INTEL_BRAND_STRING_MORE,
    CPUID_INTEL_BRAND_STRING_END,
};

__always_inline void _i686_cpuid(enum cpuid_request request, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
{
    __cpuid(request, *eax, *ebx, *ecx, *edx);
}

#endif // __EMOS_ASM_CPUID_H__
