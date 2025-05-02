/*
    From now on, What should we do is initialize the devices needed to load the
    kernel then load and jump to it. There's no need to detect and initialize
    all devices, since the kernel will do the rest.
*/

#include <stdint.h>
#include <stddef.h>

#include "asm/bootinfo.h"
#include "asm/bios/disk.h"
#include "asm/bios/video.h"
#include "asm/bios/keyboard.h"
#include "asm/bios/mem.h"

#include "bus/pci/scan.h"

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

    _bus_pci_host_scan(pci_devices, 32);

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

    struct vbe_controller_info vbe_info;
    _pc_bios_get_vbe_controller_info(&vbe_info);

    char *oem_string = (char*)(vbe_info.oem_string.segment << 4) + vbe_info.oem_string.offset;
    for (int i = 0; oem_string[i]; i++) {
        _pc_bios_tty_output(oem_string[i]);
    }
    _pc_bios_tty_output('\r');
    _pc_bios_tty_output('\n');

    uint16_t *mode_list = (uint16_t*)((vbe_info.video_modes.segment << 4) + vbe_info.video_modes.offset);
    struct vbe_video_mode_info vbe_mode_info;
    uint16_t mode = 0xFFFF;
    for (int i = 0; mode_list[i] != 0xFFFF; i++) {
        _pc_bios_get_vbe_video_mode_info(mode_list[i], &vbe_mode_info);

        if (vbe_mode_info.width == 640 && vbe_mode_info.height == 480 && vbe_mode_info.bpp == 24) {
            mode = mode_list[i];
            break;
        }
    }

    if (mode == 0xFFFF) {
        for (;;) {}
    }

    _pc_bios_set_vbe_video_mode(mode);

    uint8_t *framebuffer = (uint8_t*)(vbe_mode_info.framebuffer);
    for (int y = 0; y < vbe_mode_info.height; y++) {
        for (int x = 0; x < vbe_mode_info.width; x++) {
            framebuffer[y * vbe_mode_info.pitch + x * vbe_mode_info.bpp / 8 + 0] = 0xFF;
            framebuffer[y * vbe_mode_info.pitch + x * vbe_mode_info.bpp / 8 + 1] = 0xFF;
            framebuffer[y * vbe_mode_info.pitch + x * vbe_mode_info.bpp / 8 + 2] = 0x00;
        }
    }

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
