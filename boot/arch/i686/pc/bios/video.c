#include "video.h"

#include "bioscall.h"

void _pc_bios_tty_output(uint8_t ch)
{
    struct bioscall_regs regs = {
        .a.b.h = 0x0E,
        .a.b.l = ch
    };

    _pc_bios_call(0x10, &regs);
}
