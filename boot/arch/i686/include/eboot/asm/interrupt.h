#ifndef __EBOOT_ASM_INTERRUPT_H__
#define __EBOOT_ASM_INTERRUPT_H__

#include <stdint.h>

#include <eboot/asm/intrinsics/misc.h>

#include <eboot/compiler.h>

struct interrupt_frame {
    uint32_t error;
    uint32_t eip;
    uint16_t cs;
    uint16_t reserved1;
    uint32_t eflags;
};

#define interrupt_disable   _i686_interrupt_disable
#define interrupt_enable    _i686_interrupt_enable

#endif // __EBOOT_ASM_INTERRUPT_H__
