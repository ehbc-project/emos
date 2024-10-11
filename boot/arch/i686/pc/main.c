/*
    From now on, What should we do is initialize the devices needed to load the
    kernel then load and jump to it. There's no need to initialize all devices,
    since the kernel will do the rest.
*/

#include <stdint.h>

#include "bioscall.h"
#include "disk.h"
#include "video.h"
#include "bootinfo.h"

__attribute__((noreturn))
void _pc_init(void)
{
    const char str[] = "Hello, World!\r\n";
    for (int i = 0; str[i]; i++) {
        _pc_bios_tty_output(str[i]);
    }

    _pc_bios_read_drive_chs(_pc_boot_drive, (struct chs){ 0, 0, 1 }, 18, (void*)0x10000);
    
    
    for (;;) {}
}
