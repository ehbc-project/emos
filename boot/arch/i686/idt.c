#include "idt.h"

#include <stdint.h>

void _i686_idt_set_entry(struct idt_entry *entry, void (*handler)(void), uint16_t segment_selector, uint16_t attributes)
{
    const uint32_t handler_offset = (uint32_t)handler;
    entry->offset_low = handler_offset & 0xFFFF;
    entry->segment_selector = segment_selector;
    entry->attributes = attributes;
    entry->offset_middle = (handler_offset >> 16) & 0xFFFF;
    entry->offset_high = 0;
    entry->reserved2 = 0;
}
