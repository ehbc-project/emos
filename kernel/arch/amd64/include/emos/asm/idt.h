#ifndef __EMOS_ASM_IDT_H__
#define __EMOS_ASM_IDT_H__

#include <stdint.h>

#include <emos/compiler.h>

struct idtr {
    uint16_t size;
    uint32_t idt_ptr;
} __packed;

struct idt_entry {
    uint16_t offset_low;
    uint16_t segment_selector;
    uint8_t reserved1;
    uint8_t attributes;
    uint16_t offset_high;
} __packed;

#endif // __EMOS_ASM_IDT_H__
