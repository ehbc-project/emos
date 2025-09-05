#ifndef __I686_IDT_H__
#define __I686_IDT_H__

#include <stdint.h>

#include <asm/isr.h>

struct idt_entry {
    uint16_t offset_low;
    uint16_t segment_selector;
    uint8_t reserved1;
    uint8_t attributes;
    uint16_t offset_high;
} __attribute__((packed));

#endif // __I686_IDT_H__
