#ifndef __EBOOT_ASM_IDT_H__
#define __EBOOT_ASM_IDT_H__

#include <stdint.h>

#include <eboot/compiler.h>

struct idt_entry {
    uint16_t offset_low;
    uint16_t segment_selector;
    uint8_t reserved1;
    uint8_t attributes;
    uint16_t offset_high;
} __packed;

#endif // __EBOOT_ASM_IDT_H__
