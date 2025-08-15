#include <asm/reboot.h>

#include <stdint.h>
#include <core/panic.h>
#include <asm/io.h>

void _i686_pc_reboot()
{
    uint8_t status;
    do {
        status = _i686_in8(0x0064);
    } while (status & 0x02);
    _i686_out8(0x0064, 0xFE);

    panic("Reboot failed");
}
