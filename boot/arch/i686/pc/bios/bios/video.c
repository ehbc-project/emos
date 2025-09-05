#include <asm/bios/video.h>

#include <asm/bios/bioscall.h>

int _pc_bios_set_video_mode(uint8_t mode)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x00,
        .a.b.l = mode
    };

    return _pc_bios_call(0x10, &regs) ? regs.a.b.h : 0;
}

void _pc_bios_set_text_cursor_shape(uint16_t shape)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x01,
        .c.w = shape
    };

    _pc_bios_call(0x10, &regs);
}

void _pc_bios_set_text_cursor_pos(uint8_t page, uint8_t row, uint8_t col)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x02,
        .b.b.h = page,
        .d.b.h = row,
        .d.b.l = col,
    };

    _pc_bios_call(0x10, &regs);
}

void _pc_bios_get_text_cursor(uint8_t page, uint16_t *shape, uint8_t *row, uint8_t *col)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x03,
        .b.b.h = page,
    };

    _pc_bios_call(0x10, &regs);

    if (shape) *shape = regs.c.w;
    if (row) *row = regs.d.b.h;
    if (col) *col = regs.d.b.l;
}

void _pc_bios_tty_output(uint8_t ch)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x0E,
        .a.b.l = ch,
        .b.b.h = 0x00,
        .b.b.l = 0x0F,
    };

    _pc_bios_call(0x10, &regs);
}

void _pc_bios_write_pixel(uint8_t page, uint16_t x, uint16_t y, uint8_t color)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x0C,
        .a.b.l = color,
        .d.b.h = page,
        .c.w = x,
        .d.w = y,
    };

    _pc_bios_call(0x10, &regs);
}

uint8_t _pc_bios_read_pixel(uint8_t page, uint16_t x, uint16_t y)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x0D,
        .d.b.h = page,
        .c.w = x,
        .d.w = y,
    };

    _pc_bios_call(0x10, &regs);

    return regs.a.b.l;
}

int _pc_bios_get_vbe_controller_info(struct vbe_controller_info *buf)
{
    struct bioscall_regs regs = {
        .a.w = 0x4F00,
        .es.w = ((uint32_t)buf >> 4) & 0xFFFF,
        .di.w = (uint32_t)buf & 0x000F,
    };

    _pc_bios_call(0x10, &regs);

    if (regs.a.w != 0x004F) return 1;

    return 0;
}

int _pc_bios_get_vbe_video_mode_info(uint16_t mode, struct vbe_video_mode_info *buf)
{
    struct bioscall_regs regs = {
        .a.w = 0x4F01,
        .c.w = mode,
        .es.w = ((uint32_t)buf >> 4) & 0xFFFF,
        .di.w = (uint32_t)buf & 0x000F,
    };

    _pc_bios_call(0x10, &regs);

    if (regs.a.w != 0x004F) return 1;

    return 0;
}

int _pc_bios_set_vbe_video_mode(uint16_t mode)
{
    struct bioscall_regs regs = {
        .a.w = 0x4F02,
        .b.w = mode
    };

    _pc_bios_call(0x10, &regs);

    if (regs.a.w != 0x004F) return 1;

    return 0;
}

int _pc_bios_set_vbe_display_start(uint16_t x, uint16_t y)
{
    struct bioscall_regs regs = {
        .a.w = 0x4F07,
        .b.b.h = 0x00,
        .b.b.l = 0x00,
        .c.w = x,
        .d.w = y,
    };

    return _pc_bios_call(0x10, &regs);
}

int _pc_bios_set_vbe_display_start_at_retrace(uint16_t x, uint16_t y)
{
    struct bioscall_regs regs = {
        .a.w = 0x4F07,
        .b.b.h = 0x00,
        .b.b.l = 0x80,
        .c.w = x,
        .d.w = y,
    };

    return _pc_bios_call(0x10, &regs);
}

int _pc_bios_schedule_vbe_display_start(uint32_t fboffset)
{
    struct bioscall_regs regs = {
        .a.w = 0x4F07,
        .b.b.h = 0x00,
        .b.b.l = 0x02,
        .c.l = fboffset,
    };

    return _pc_bios_call(0x10, &regs);
}

int _pc_bios_schedule_vbe_display_start_at_retrace(uint32_t fboffset)
{
    struct bioscall_regs regs = {
        .a.w = 0x4F07,
        .b.b.h = 0x00,
        .b.b.l = 0x82,
        .c.l = fboffset,
    };

    return _pc_bios_call(0x10, &regs);
}

int _pc_bios_get_vbe_pm_interface(farptr_t *pmi_table)
{
    struct bioscall_regs regs = {
        .a.w = 0x4F0A,
        .b.b.l = 0x00,
    };

    _pc_bios_call(0x10, &regs);
    
    if (pmi_table) {
        pmi_table->segment = regs.es.w;
        pmi_table->offset = regs.di.w;
    }
    
    return regs.a.w;
}

int _pc_bios_get_vbe_edid(uint16_t ctrlr_unit, uint16_t edid_block, struct edid *buf)
{
    struct bioscall_regs regs = {
        .a.w = 0x4F15,
        .b.b.l = 0x01,
        .c.w = ctrlr_unit,
        .d.w = edid_block,
        .es.w = ((uint32_t)buf >> 4) & 0xFFFF,
        .di.w = (uint32_t)buf & 0x000F,
    };

    return _pc_bios_call(0x10, &regs);
}
