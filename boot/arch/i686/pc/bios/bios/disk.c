#include "asm/bios/disk.h"

#include <stdint.h>

#include "asm/bios/bioscall.h"

struct dap {
    uint8_t dap_size;
    uint8_t unused;
    uint16_t count;
    uint16_t buffer_offset;
    uint16_t buffer_segment;
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

int _pc_bios_read_drive(uint8_t drive, struct chs chs, uint8_t count, void *buf)
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

int _pc_bios_write_drive(uint8_t drive, struct chs chs, uint8_t count, const void *buf)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x03,
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

int _pc_bios_get_drive_params(uint8_t drive, enum bios_drive_type *type, struct chs *geometry, struct bios_disk_base_table **dbt)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x08,
        .d.b.l = drive,
        .es.w = 0,
        .di.w = 0,
    };

    int err = _pc_bios_call(0x13, &regs);
    if (err) return err;

    if (type) *type = regs.b.b.l;
    if (geometry) {
        geometry->head = (int)regs.d.b.h + 1;
        geometry->cylinder = (((int)regs.c.b.l >> 6) | (int)regs.c.b.h) + 1;
        geometry->sector = (int)regs.c.b.l & 0x3F;
    }
    if (dbt && !(drive & 0x80)) {
        *dbt = (void *)(((uint32_t)regs.es.w << 4) + regs.di.w);
    }

    return 0;
}

int _pc_bios_check_ext(uint8_t drive, enum bios_edd_version *edd_version, uint16_t *subset_support_flags)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x41,
        .b.w = 0x55AA,
        .d.b.l = drive,
    };

    int err = _pc_bios_call(0x13, &regs);
    if (err) return err;

    if (regs.b.w != 0xAA55) {
        return 1;
    }

    if (edd_version) *edd_version = regs.a.b.h;
    if (subset_support_flags) *subset_support_flags = regs.c.w;
    
    return 0;
}

int _pc_bios_read_drive_ext(uint8_t drive, lba_t lba, uint16_t count, void *buf)
{
    struct dap dap = {
        .dap_size = sizeof(struct dap),
        .count = count,
        .buffer_offset = (uint32_t)buf & 0x000F,
        .buffer_segment = ((uint32_t)buf >> 4) & 0xFFFF,
        .lba_low = lba & 0xFFFFFFFF,
        .lba_high = lba >> 32,
    };

    struct bioscall_regs regs = {
        .a.b.h = 0x42,
        .d.b.l = drive,
        .ds.w = ((uint32_t)&dap >> 4) & 0xFFFF,
        .si.w = (uint32_t)&dap & 0x000F,
    };
    return _pc_bios_call(0x13, &regs);
}

int _pc_bios_write_drive_ext(uint8_t drive, lba_t lba, uint16_t count, const void *buf)
{
    struct dap dap = {
        .dap_size = sizeof(struct dap),
        .count = count,
        .buffer_offset = (uint32_t)buf & 0x000F,
        .buffer_segment = ((uint32_t)buf >> 4) & 0xFFFF,
        .lba_low = lba & 0xFFFFFFFF,
        .lba_high = lba >> 32,
    };

    struct bioscall_regs regs = {
        .a.b.h = 0x43,
        .d.b.l = drive,
        .ds.w = ((uint32_t)&dap >> 4) & 0xFFFF,
        .si.w = (uint32_t)&dap & 0x000F,
    };
    return _pc_bios_call(0x13, &regs);
}

int _pc_bios_get_drive_params_ext(uint8_t drive, struct bios_extended_drive_params *params)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x48,
        .d.b.l = drive,
        .ds.w = ((uint32_t)params >> 4) & 0xFFFF,
        .si.w = (uint32_t)params & 0x000F,
    };

    return _pc_bios_call(0x13, &regs);
}

