#include <asm/poweroff.h>

#include <stdint.h>
#include <debug.h>
#include <sys/io.h>

void _i686_pc_poweroff()
{
    io_out16(0xB004, 0x2000);
    io_out16(0x0604, 0x2000);
    io_out16(0x4004, 0x3400);

    panic("Poweroff failed");
}
