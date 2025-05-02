#ifndef __I686_IDT_H__
#define __I686_IDT_H__

#include <stdint.h>

__attribute__((packed))
struct idt_entry {
    uint16_t offset_low;
    uint16_t segment_selector;
    uint16_t attributes;
    uint16_t offset_middle;
    uint32_t offset_high;
    uint32_t reserved2;
};

void _i686_idt_set_entry(struct idt_entry *entry, void (*handler)(void), uint16_t segment_selector, uint16_t attributes);
void _i686_idt_load(struct idt_entry *idt, long idt_size);

#endif // __I686_IDT_H__
