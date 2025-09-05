#include <stdio.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <interface/char.h>
#include <interface/console.h>
#include <interface/framebuffer.h>
#include <interface/hid.h>
#include <asm/bios/misc.h>
#include <shell.h>

extern int __text_start;
extern int __data_start;
extern int __bss_start;

void main(void)
{
    struct device *fbdev = find_device("video0");
    const struct framebuffer_interface *fbi = fbdev->driver->get_interface(fbdev, "framebuffer");

    struct device *vcdev;
    if (fbi) {
        fbi->set_mode(fbdev, 640, 480, 24);
        vcdev = mm_allocate_clear(1, sizeof(struct device));
        vcdev->parent = fbdev;
        vcdev->driver = find_device_driver("vconsole");
        register_device(vcdev);
    } else {
        vcdev = fbdev;
    }
    const struct console_interface *vci = vcdev->driver->get_interface(vcdev, "console");
    vci->set_dimension(vcdev, 80, fbdev == vcdev ? 25 : 30);

    struct device *ttydev = mm_allocate_clear(1, sizeof(struct device));
    ttydev->parent = vcdev;
    ttydev->driver = find_device_driver("ansiterm");
    register_device(ttydev);

    freopen_device(stdout, "tty0");
    freopen_device(stderr, "tty0");
    freopen_device(stdin, "kbd0");

    printf("\x1b[3J\x1b[0;0f\x1b[?25lEMOS Bootloader\n");
    printf("═══════════════\n");
    printf("\xea");
    printf("\xb0");
    printf("\x80");
    printf("\n");

    // printf(".text=0x%p, .data=0x%p, .bss=0x%p\n", (void *)&__text_start, (void *)&__data_start, (void *)&__bss_start);

    // /* List Memory Map */
    // uint32_t cursor = 0;
    // struct smap_entry entry;
    // 
    // printf("Memory Map: \r\n");
    // do {
    //     if (_pc_bios_query_address_map(&cursor, &entry, sizeof(entry)) < 1) {
    //         break;
    //     }
    //     printf("\t0x%08lX 0x%08lX 0x%08lX\r\n", entry.base_addr_low, entry.length_low, entry.type);
    // } while (cursor);

    struct device *kbd = find_device("kbd0");
    const struct hid_interface *hidi = kbd->driver->get_interface(kbd, "hid");
    uint16_t key, flags;

    int selection = 0, selected = 0;
    static const char *options[] = {
        "Boot from next device",
        "Boot from current device",
        "Start bootloader shell",
    };

    while (!selected) {
        hidi->wait_event(kbd);
        if (hidi->poll_event(kbd, &key, &flags) || (flags & 1)) {
            continue;
        }
        switch (key)  {
            case KEY_UP:
                if (selection > 0) {
                    selection--;
                }
                break;
            case KEY_DOWN:
                if (selection < sizeof(options) / sizeof(*options) - 1) {
                    selection++;
                }
                break;
            case KEY_ENTER:
            case KEY_KPENTER:
                selected = 1;
                break;
            default:
                break;
        }

        for (int i = 0; i < sizeof(options) / sizeof(*options); i++) {
            if (selection == i) {
                printf("\x1b[7m%s\x1b[0m\n", options[i]);
            } else {
                printf("%s\n", options[i]);
            }
        }
        printf("\x1b[%ldA", sizeof(options) / sizeof(*options));
    }

    switch (selection) {
        case 0:
            _pc_bios_bootnext();
            break;
        case 1:
            break;
        case 2:
            start_shell();
            break;
    }

    for (;;) {}
}

