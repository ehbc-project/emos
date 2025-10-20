#ifndef __I686_PC_ISR_H__
#define __I686_PC_ISR_H__

#include <stdint.h>

#include <device/device.h>
#include <asm/interrupt.h>

struct trap_regs {
    uint16_t ss;
    uint16_t gs;
    uint16_t fs;
    uint16_t es;
    uint16_t ds;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
} __packed;

typedef void (*interrupt_handler_t)(struct device *, int);
typedef void (*trap_handler_t)(struct interrupt_frame *, struct trap_regs *, int, int);

struct isr_table_entry {
    struct isr_table_entry *next;
    struct device *dev;
    int is_interrupt;
    union {
        interrupt_handler_t interrupt_handler;
        trap_handler_t trap_handler;
    };
};

void _pc_init_idt(void);
void _pc_set_interrupt_handler(int num, struct device *dev, interrupt_handler_t func);
void _pc_set_trap_handler(int num, trap_handler_t func);

struct isr_table_entry *_pc_get_isr_table_entry(int num);

uint64_t _pc_get_irq_count(void);

#endif // __I686_PC_ISR_H__
