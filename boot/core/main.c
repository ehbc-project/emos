#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <zlib.h>

#include <eboot/asm/bios/bootinfo.h>
#include <eboot/asm/bios/misc.h>

#include <eboot/macros.h>
#include <eboot/status.h>
#include <eboot/debug.h>
#include <eboot/device.h>
#include <eboot/filesystem.h>
#include <eboot/json.h>
#include <eboot/hid.h>
#include <eboot/interface/block.h>
#include <eboot/interface/char.h>
#include <eboot/interface/console.h>
#include <eboot/interface/framebuffer.h>
#include <eboot/interface/hid.h>

#include <x86gprintrin.h>

#include "shell/shell.h"

void main(void)
{
    status_t status;

    struct device *fbdev;
    status = device_find("video0", &fbdev);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "find_device() failed: 0x%08X\n", status);
        panic("find_device() failed");
    }
    
    const struct framebuffer_interface *fbi;
    status = fbdev->driver->get_interface(fbdev, "framebuffer", (const void **)&fbi);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "get_interface() failed: 0x%08X\n", status);
        panic("get_interface() failed");
    }

    struct device *condev;
    if (fbi) {
        struct device_driver *condrv;

        fbi->set_mode(fbdev, 640, 480, 24);
        
        status = device_driver_find("vconsole", &condrv);
        if (!CHECK_SUCCESS(status)) {
            fprintf(stderr, "find_device_driver() failed: 0x%08X\n", status);
            panic("failed to initialize essential virtual device");
        }

        status = condrv->probe(&condev, condrv, fbdev, NULL, 0);
        if (!CHECK_SUCCESS(status)) {
            fprintf(stderr, "register_device() failed: 0x%08X\n", status);
            panic("failed to initialize essential virtual device");
        }

        const struct console_interface *vci;
        status = condev->driver->get_interface(condev, "console", (const void **)&vci);
        if (!CHECK_SUCCESS(status)) {
            fprintf(stderr, "get_interface() failed: 0x%08X\n", status);
            panic("failed to get interface from device");
        }

        status = vci->set_dimension(condev, 80, 30);
        if (!CHECK_SUCCESS(status)) {
            fprintf(stderr, "set_dimension() failed: 0x%08X\n", status);
            panic("failed to set up device");
        }
    } else {
        condev = fbdev;

        const struct console_interface *vci;
        status = condev->driver->get_interface(condev, "console", (const void **)&vci);
        if (!CHECK_SUCCESS(status)) {
            fprintf(stderr, "get_interface() failed: 0x%08X\n", status);
            panic("failed to get interface from device");
        }

        vci->set_dimension(condev, 80, 25);
        if (!CHECK_SUCCESS(status)) {
            fprintf(stderr, "set_dimension() failed: 0x%08X\n", status);
            panic("failed to set up device");
        }
    }

    struct device *ttydev;
    struct device_driver *ttydrv;

    status = device_driver_find("ansiterm", &ttydrv);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "find_device_driver() failed: 0x%08X\n", status);
        panic("failed to initialize essential virtual device");
    }

    status = ttydrv->probe(&ttydev, ttydrv, condev, NULL, 0);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "register_device() failed: 0x%08X\n", status);
        panic("failed to initialize essential virtual device");
    }

    if (freopendevice("tty0", stdout)) {
        panic("failed to reopen basic file");
    }
    if (freopendevice("tty0", stderr)) {
        panic("failed to reopen basic file");
    }
    if (freopendevice("kbd0", stdin)) {
        panic("failed to reopen basic file");
    }

    struct device *bootdisk = device_get_first_dev();
    const struct block_interface *blki = NULL;
    uint32_t bootsect_crc32 = crc32(0, _pc_boot_sector, sizeof(_pc_boot_sector));;
    for (; bootdisk; bootdisk = bootdisk->next) {
        status = bootdisk->driver->get_interface(bootdisk, "block", (const void **)&blki);
        if (!CHECK_SUCCESS(status)) continue;

        uint8_t sect0[512];

        status = blki->read(bootdisk, 0, sect0, 1, NULL);
        if (!CHECK_SUCCESS(status)) {
            fprintf(stderr, "read() failed: 0x%08X\n", status);
            continue;
        }

        if (memcmp(_pc_boot_sector, sect0, sizeof(sect0)) == 0) break;
    }

    if (!bootdisk) {
        panic("cannot found boot disk");
    }

    hexdump(_pc_boot_sector, sizeof(_pc_boot_sector), 0);

    status = filesystem_auto_mount(bootdisk, "boot");
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "fs_auto_mount() failed: 0x%08X\n", status);
        panic("failed to mount boot disk");
    }

    FILE *cfg_fp = fopen("boot:/config/boot.json", "r");
    if (!cfg_fp) {
        panic("boot:/config/boot.json not found");
    }

    fseek(cfg_fp, 0, SEEK_END);
    long cfg_len = ftell(cfg_fp);
    fseek(cfg_fp, 0, SEEK_SET);

    char *cfg_str = malloc(cfg_len);
    fread(cfg_str, cfg_len, 1, cfg_fp);

    struct json_value *json;
    status = json_parse(cfg_str, cfg_len, &json);
    if (!CHECK_SUCCESS(status) || !json || json->type != JVT_OBJECT) {
        panic("invalid config file");
    }
    
    fclose(cfg_fp);
    
    struct json_value *title;
    status = json_object_find_value(&json->obj, "title", &title);
    if (!CHECK_SUCCESS(status) || !title) {
        panic("element \"title\" not found");
    } else if (title->type != JVT_STRING) {
        panic("element \"title\" has invalid value type");
    }

    struct json_value *start_script;
    status = json_object_find_value(&json->obj, "start_script", &start_script);
    if (!CHECK_SUCCESS(status) || !start_script) {
        // skip running startup script
    } else if (start_script->type == JVT_STRING) {
        shell_execute(NULL, start_script->str);
    } else if (start_script->type == JVT_ARRAY) {
        for (struct json_array_elem *elem = start_script->arr.elem; elem; elem = elem->next) {
            if (elem->value->type != JVT_STRING) {
                panic("an element of array \"start_script\" has invalid value type");
            }
            shell_execute(NULL, elem->value->str);
        }
    } else {
        panic("element \"start_script\" has invalid value type");
    }

    printf("\x1b[3J\x1b[0;0f\x1b[?25l%s\n", title->str);
    printf("════════════════════════════════════════\n");

    struct device *kbd;
    status = device_find("kbd0", &kbd);
    if (!CHECK_SUCCESS(status)) {
        panic("keyboard not detected");
    }

    const struct hid_interface *hidi;
    status = kbd->driver->get_interface(kbd, "hid", (const void **)&hidi);
    if (!CHECK_SUCCESS(status)) {
        panic("failed to get interface from device");
    }

    struct json_value *options;
    status = json_object_find_value(&json->obj, "options", &options);
    if (!CHECK_SUCCESS(status) || !options) {
        panic("element \"options\" not found");
    } else if (options->type != JVT_ARRAY) {
        panic("element \"options\" has invalid value type");
    }
    
    unsigned int option_count;
    status = json_array_get_element_count(&options->arr, &option_count);
    if (!CHECK_SUCCESS(status)) {
        panic("element \"options\" has invalid value type");
    }

    int selection = 0, selected = 0;
    uint16_t key, flags;
    while (!selected) {
        status = hidi->wait_event(kbd);
        if (!CHECK_SUCCESS(status)) continue;

        status = hidi->poll_event(kbd, &key, &flags);
        if (!CHECK_SUCCESS(status) || (flags & 1)) continue;

        switch (key)  {
            case KEY_UP:
                if (selection > 0) {
                    selection--;
                }
                break;
            case KEY_DOWN:
                if (selection < option_count - 1) {
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

        for (int i = 0; i < option_count; i++) {
            struct json_value *option;
            status = json_array_find_value(&options->arr, i, &option);
            if (!CHECK_SUCCESS(status) || !option || option->type != JVT_OBJECT) {
                panic("an element of array \"options\" is not found or has invalid type");
            }

            struct json_value *option_name;
            status = json_object_find_value(&option->obj, "name", &option_name);
            if (!CHECK_SUCCESS(status) || !option_name || option_name->type != JVT_STRING) {
                panic("element \"name\" is not found or has invalid type");
            }

            if (selection == i) {
                printf("\x1b[7m%s\x1b[0m\n", option_name->str);
            } else {
                printf("%s\n", option_name->str);
            }
        }
        printf("\x1b[%uA", option_count);
    }

    struct json_value *option;
    status = json_array_find_value(&options->arr, selection, &option);
    if (!CHECK_SUCCESS(status) || !option || option->type != JVT_OBJECT) {
        panic("an element of array \"options\" is not found or has invalid type");
    }

    fputs("\x1b[?25h\n", stdout);

    struct json_value *option_script;
    status = json_object_find_value(&option->obj, "script", &option_script);
    if (!CHECK_SUCCESS(status) || !option_script) {
        panic("element \"script\" is not available");
    } else if (option_script->type == JVT_STRING) {
        shell_execute(NULL, option_script->str);
    } else if (option_script->type == JVT_ARRAY) {
        for (struct json_array_elem *elem = option_script->arr.elem; elem; elem = elem->next) {
            if (elem->value->type != JVT_STRING) {
                panic("an element of array \"script\" has invalid value type");
            }
            shell_execute(NULL, elem->value->str);
        }
    } else {
        panic("element \"script\" has invalid value type");
    }

    json_destruct(json);

    for (;;) {}
}

