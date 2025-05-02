#include "asm/bios/disk.h"

#include <stdint.h>

#include "asm/bios/bioscall.h"

struct dap {
    uint8_t dap_size;
    uint8_t unused;
    uint16_t count;
    uint16_t buffer_segment;
    uint16_t buffer_offset;
    uint32_t lba_low;
    uint32_t lba_high;
};

int _pc_bios_reset_drive(uint8_t drive)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x00,
        .d.b.l = drive
    };
    return _pc_bios_call(0x13, &regs) ? regs.a.b.h : 0;
}

int _pc_bios_read_drive_chs(uint8_t drive, struct chs chs, uint8_t count, void *buf)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x02,
        .a.b.l = count,
        .c.b.h = chs.cylinder & 0xFF,
        .c.b.l = (((chs.cylinder >> 8) & 0x3) << 6) | (chs.sector & 0x3F),
        .d.b.h = chs.head,
        .d.b.l = drive,
        .es.w = ((uint32_t)buf >> 4) & 0xFFFF,
        .b.w = (uint32_t)buf & 0x000F,
    };
    return _pc_bios_call(0x13, &regs) ? -(regs.a.b.h) : regs.a.b.l;
}



