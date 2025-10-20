#include <stdio.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <interface/char.h>
#include <interface/console.h>
#include <interface/framebuffer.h>
#include <interface/hid.h>
#include <asm/bios/misc.h>
#include <macros.h>
#include <shell.h>
#include <zlib.h>
#include <debug.h>

#include <x86gprintrin.h>

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

    freopendevice("tty0", stdout);
    freopendevice("tty0", stderr);
    freopendevice("kbd0", stdin);

    printf("\x1b[3J\x1b[0;0f\x1b[?25lEMOS Bootloader\n");
    printf("═══════════════\n");
    
    uint64_t ts = __rdtsc();
    printf("%016llX\n", ts);

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
                if (selection < ARRAY_SIZE(options) - 1) {
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

        for (int i = 0; i < ARRAY_SIZE(options); i++) {
            if (selection == i) {
                printf("\x1b[7m%s\x1b[0m\n", options[i]);
            } else {
                printf("%s\n", options[i]);
            }
        }
        printf("\x1b[%ldA", ARRAY_SIZE(options));
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

