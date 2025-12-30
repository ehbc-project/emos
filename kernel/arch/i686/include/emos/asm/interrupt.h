#ifndef __EMOS_ASM_INTERRUPT_H__
#define __EMOS_ASM_INTERRUPT_H__

#include <stdint.h>

#include <compiler.h>

struct interrupt_frame {
    uint32_t eip;
    uint16_t cs;
    uint16_t reserved1;
    uint32_t eflags;
};

__always_inline void interrupt_enable(void)
{
    asm volatile ("sti");
}

__always_inline void interrupt_disable(void)
{
    asm volatile ("cli");
}

#endif // __EMOS_ASM_INTERRUPT_H__
