#include "keyboard.h"

#include "bioscall.h"

void _pc_bios_read_keyboard(uint8_t *scancode, char *ascii)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x10
    };

    _pc_bios_call(0x16, &regs);

    if (scancode) *scancode = regs.a.b.h;
    if (ascii) *ascii = regs.a.b.l;
}

int _pc_bios_read_keyboard_nowait(uint8_t *scancode, char *ascii)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x11
    };

    _pc_bios_call(0x16, &regs);

    if (!regs.a.w) return 1;

    if (scancode) *scancode = regs.a.b.h;
    if (ascii) *ascii = regs.a.b.l;

    return 0;
}

uint16_t _pc_bios_read_keyboard_flags(void)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x12
    };

    _pc_bios_call(0x16, &regs);

    return regs.a.w;
}

