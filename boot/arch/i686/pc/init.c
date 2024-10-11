/*
    From now on, What should we do is initialize the devices needed to load the
    kernel then load and jump to it. There's no need to detect and initialize
    all devices, since the kernel will do the rest.
*/

#include <stdint.h>
#include <stddef.h>

#include "disk.h"
#include "video.h"
#include "bootinfo.h"
#include "keyboard.h"

extern void main(void);

__attribute__((noreturn))
void _pc_init(void)
{
    const char str[] = "Hello, World!\r\n";
    for (int i = 0; str[i]; i++) {
        _pc_bios_tty_output(str[i]);
    }

    _pc_bios_read_drive_chs(_pc_boot_drive, (struct chs){ 0, 0, 1 }, 18, (void*)0x10000);

    main();

    for (;;) {}
}
