#ifndef __I686_PC_ISR_H__
#define __I686_PC_ISR_H__

#include <stdint.h>

#include <device/device.h>
#include <asm/interrupt.h>

struct pushal_regs {
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
};

typedef void (*interrupt_handler_t)(struct device *, int);
typedef void (*trap_handler_t)(struct interrupt_frame *, struct pushal_regs *, int, int);

void _pc_init_idt(void);
void _pc_set_interrupt_handler(int num, struct device *dev, interrupt_handler_t func);
void _pc_set_trap_handler(int num, struct device *dev, trap_handler_t func);

uint64_t _pc_get_irq_count(void);

#endif // __I686_PC_ISR_H__
