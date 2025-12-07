#include <eboot/asm/power.h>

#include <stdint.h>

#include <eboot/asm/io.h>

#include <eboot/debug.h>

void _pc_reboot()
{
    uint8_t status;
    do {
        status = io_in8(0x0064);
    } while (status & 0x02);
    io_out8(0x0064, 0xFE);

    panic("Reboot failed");
}

void _pc_poweroff()
{
    io_out16(0xB004, 0x2000);
    io_out16(0x0604, 0x2000);
    io_out16(0x4004, 0x3400);

    panic("Poweroff failed");
}
