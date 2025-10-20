#ifndef __X86_64_INTERRUPT_H__
#define __X86_64_INTERRUPT_H__

#include <stdint.h>

struct interrupt_frame {
    uint32_t eip;
    uint16_t cs;
    uint16_t reserved1;
    uint32_t eflags;
};

void _x86_64_enable_interrupt(void);
void _x86_64_disable_interrupt(void);

#endif // __X86_64_INTERRUPT_H__
