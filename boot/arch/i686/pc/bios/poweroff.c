#include <asm/poweroff.h>

#include <stdint.h>
#include <core/panic.h>
#include <asm/io.h>

void _i686_pc_poweroff()
{
    _i686_out16(0xB004, 0x2000);
    _i686_out16(0x0604, 0x2000);
    _i686_out16(0x4004, 0x3400);

    panic("Poweroff failed");
}
