/*
    From now on, What should we do is initialize the devices needed to load the
    kernel then load and jump to it. There's no need to detect and initialize
    all devices, since the kernel will do the rest.
*/

#include <stdint.h>
#include <stddef.h>

#include "bootinfo.h"
#include "bios/disk.h"
#include "bios/video.h"
#include "bios/keyboard.h"
#include "bios/mem.h"

#include "pci/cfgspace.h"
#include "pci/scan.h"
extern void main(void);

struct pci_device pci_devices[32];


static void print_hex_dword(uint32_t value)
{
    static const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 0; i < 8; i++) {
        _pc_bios_tty_output(hex_chars[(value >> (28 - i * 4)) & 0xF]);
    }
}

__attribute__((noreturn))
void _pc_init(void)
{
    const char str[] = "Hello, World!\r\n";
    for (int i = 0; str[i]; i++) {
        _pc_bios_tty_output(str[i]);
    }

    _pc_bios_read_drive_chs(_pc_boot_drive, (struct chs){ 0, 0, 1 }, 18, (void*)0x10000);

    _pc_pci_host_scan(pci_devices, 32);

    uint32_t cursor = 0;
    struct smap_entry entry;

    do {
        _pc_bios_query_address_map(&cursor, &entry, sizeof(entry));
        print_hex_dword(entry.base_addr_low);
        _pc_bios_tty_output(' ');
        print_hex_dword(entry.length_low);
        _pc_bios_tty_output(' ');
        print_hex_dword(entry.type);
        _pc_bios_tty_output('\r');
        _pc_bios_tty_output('\n');
    } while (cursor);

    for (;;) {
        char ch;
        _pc_bios_read_keyboard(NULL, &ch);
        if (ch == '\r') {
            _pc_bios_tty_output('\n');
        }
        _pc_bios_tty_output(ch);
    }

    main();

    for (;;) {}
}
