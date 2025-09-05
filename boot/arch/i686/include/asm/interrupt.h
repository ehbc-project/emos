#ifndef __I686_INTERRUPT_H__
#define __I686_INTERRUPT_H__

#include <stdint.h>

struct interrupt_frame {
    uint32_t eip;
    uint16_t cs;
    uint16_t reserved1;
    uint32_t eflags;
};

void _i686_enable_interrupt(void);
void _i686_disable_interrupt(void);

#endif // __I686_INTERRUPT_H__
