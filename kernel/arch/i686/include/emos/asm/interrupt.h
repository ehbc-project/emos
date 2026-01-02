#ifndef __EMOS_ASM_INTERRUPT_H__
#define __EMOS_ASM_INTERRUPT_H__

#include <stdint.h>

#include <emos/compiler.h>

struct interrupt_frame {
    uint32_t error;
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

__always_inline uint32_t interrupt_save(void)
{
    uint32_t flags;

    asm volatile (
        "pushfl\r\n"
        "pop    %0\r\n"
        : "=r"(flags)
    );

    return (flags & 0x0200) ? 1 : 0;
}

__always_inline void interrupt_restore(uint32_t state)
{
    if (state) {
        interrupt_enable();
    } else {
        interrupt_disable();
    }
}

#endif // __EMOS_ASM_INTERRUPT_H__
