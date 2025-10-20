#include <asm/reboot.h>

#include <stdint.h>
#include <debug.h>
#include <sys/io.h>

void _i686_pc_reboot()
{
    uint8_t status;
    do {
        status = io_in8(0x0064);
    } while (status & 0x02);
    io_out8(0x0064, 0xFE);

    panic("Reboot failed");
}
