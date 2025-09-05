#include "asm/bios/mem.h"

#include "asm/bios/bioscall.h"

long _pc_bios_query_address_map(uint32_t *_cursor, struct smap_entry *buf, long buf_size)
{
    uint32_t cursor = _cursor ? *_cursor : 0;

    struct bioscall_regs regs = {
        .a.l = 0xE820,
        .b.l = cursor,
        .c.l = buf_size,
        .d.l = 0x534D4150,
        .es.w = ((uint32_t)buf >> 4) & 0xFFFF,
        .di.w = (uint32_t)buf & 0x000F,
    };

    int err = _pc_bios_call(0x15, &regs);

    if (_cursor) *_cursor = regs.b.l;
    
    return err ? -1 : regs.c.l;
}
