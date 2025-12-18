#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <eboot/asm/bios/bootinfo.h>
#include <eboot/asm/bios/misc.h>
#include <eboot/asm/power.h>

#include <eboot/macros.h>
#include <eboot/status.h>
#include <eboot/panic.h>
#include <eboot/device.h>
#include <eboot/filesystem.h>
#include <eboot/json.h>
#include <eboot/hid.h>
#include <eboot/shell.h>
#include <eboot/interface/block.h>
#include <eboot/interface/char.h>
#include <eboot/interface/console.h>
#include <eboot/interface/framebuffer.h>
#include <eboot/interface/hid.h>

#include <x86gprintrin.h>

void setup_tty(void)
{
    status_t status;
    struct device *fbdev;
    const struct framebuffer_interface *fbi;
    struct device *condev;
    struct device_driver *condrv;
    const struct console_interface *vci;
    struct device *ttydev;
    struct device_driver *ttydrv;

    status = device_find("video0", &fbdev);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "find_device() failed: 0x%08X\n", status);
        panic(status, "video device not available");
    }
    
    status = fbdev->driver->get_interface(fbdev, "framebuffer", (const void **)&fbi);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "get_interface() failed: 0x%08X\n", status);
        panic(status, "failed to get interface from device");
    }
    
    status = device_driver_find("vconsole", &condrv);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "find_device_driver() failed: 0x%08X\n", status);
        panic(status, "failed to initialize essential virtual device");
    }

    status = condrv->probe(&condev, condrv, fbdev, NULL, 0);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "register_device() failed: 0x%08X\n", status);
        panic(status, "failed to initialize essential virtual device");
    }

    status = condev->driver->get_interface(condev, "console", (const void **)&vci);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "get_interface() failed: 0x%08X\n", status);
        panic(status, "failed to get interface from device");
    }

    status = device_driver_find("ansiterm", &ttydrv);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "find_device_driver() failed: 0x%08X\n", status);
        panic(status, "failed to initialize essential virtual device");
    }

    status = ttydrv->probe(&ttydev, ttydrv, condev, NULL, 0);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "register_device() failed: 0x%08X\n", status);
        panic(status, "failed to initialize essential virtual device");
    }

    if (freopendevice("tty0", stdout)) {
        panic(STATUS_UNKNOWN_ERROR, "failed to reopen essential file");
    }
    if (freopendevice("tty0", stderr)) {
        panic(STATUS_UNKNOWN_ERROR, "failed to reopen essential file");
    }
    if (freopendevice("kbd0", stdin)) {
        panic(STATUS_UNKNOWN_ERROR, "failed to reopen essential file");
    }
}

void show_menu(void)
{
    status_t status;
    FILE *cfg_fp = NULL;
    long cfg_len;
    char *cfg_str = NULL;
    struct json_value *json = NULL;
    struct json_value *title = NULL;
    struct json_value *start_script = NULL;
    struct device *kbd;
    const struct hid_interface *hidi;
    struct json_value *options;
    unsigned int option_count;
    int selection = 0, selected = 0;
    uint16_t key, flags;
    struct json_value *option;
    struct json_value *option_name;
    struct json_value *option_script;
    
    cfg_fp = fopen("boot:/config/boot.json", "r");
    if (!cfg_fp) {
        panic(STATUS_ENTRY_NOT_FOUND, "boot:/config/boot.json not found");
    }

    fseek(cfg_fp, 0, SEEK_END);
    cfg_len = ftell(cfg_fp);
    fseek(cfg_fp, 0, SEEK_SET);

    cfg_str = malloc(cfg_len);
    fread(cfg_str, cfg_len, 1, cfg_fp);

    status = json_parse(cfg_str, cfg_len, &json);
    if (!CHECK_SUCCESS(status) || !json || json->type != JVT_OBJECT) {
        panic(STATUS_INVALID_FORMAT, "invalid config file");
    }
    
    free(cfg_str);
    fclose(cfg_fp);
    
    status = json_object_find_value(&json->obj, "title", &title);
    if (!CHECK_SUCCESS(status) || !title) {
        panic(STATUS_INVALID_FORMAT, "element \"title\" not found");
    } else if (title->type != JVT_STRING) {
        panic(STATUS_INVALID_FORMAT, "element \"title\" has invalid value type");
    }

    status = json_object_find_value(&json->obj, "start_script", &start_script);
    if (!CHECK_SUCCESS(status) || !start_script) {
        // skip running startup script
    } else if (start_script->type == JVT_STRING) {
        shell_execute(NULL, start_script->str);
    } else if (start_script->type == JVT_ARRAY) {
        for (struct json_array_elem *elem = start_script->arr.elem; elem; elem = elem->next) {
            if (elem->value->type != JVT_STRING) {
                panic(STATUS_INVALID_FORMAT, "an element of array \"start_script\" has invalid value type");
            }
            shell_execute(NULL, elem->value->str);
        }
    } else {
        panic(STATUS_INVALID_FORMAT, "element \"start_script\" has invalid value type");
    }

    printf("\x1b[3J\x1b[0;0f\x1b[?25l%s\n", title->str);
    printf("════════════════════════════════════════\n");

    status = device_find("kbd0", &kbd);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "keyboard not detected");
    }
    
    status = kbd->driver->get_interface(kbd, "hid", (const void **)&hidi);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to get interface from device");
    }

    status = json_object_find_value(&json->obj, "options", &options);
    if (!CHECK_SUCCESS(status) || !options) {
        panic(!CHECK_SUCCESS(status) ? status : STATUS_INVALID_FORMAT, "element \"options\" not found");
    } else if (options->type != JVT_ARRAY) {
        panic(STATUS_INVALID_FORMAT, "element \"options\" has invalid value type");
    }

    status = json_array_get_element_count(&options->arr, &option_count);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "cannot get element count of element \"options\"");
    }

    while (!selected) {
        printf("\x1b[3;0H");
        for (int i = 0; i < option_count; i++) {
            status = json_array_find_value(&options->arr, i, &option);
            if (!CHECK_SUCCESS(status) || !option || option->type != JVT_OBJECT) {
                panic(STATUS_INVALID_FORMAT, "an element of array \"options\" is not found or has invalid type");
            }

            status = json_object_find_value(&option->obj, "name", &option_name);
            if (!CHECK_SUCCESS(status) || !option_name || option_name->type != JVT_STRING) {
                panic(STATUS_INVALID_FORMAT, "element \"name\" is not found or has invalid type");
            }

            if (selection == i) {
                printf("\x1b[7m%s\x1b[0m\n", option_name->str);
            } else {
                printf("%s\n", option_name->str);
            }
        }

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
    }

    status = json_array_find_value(&options->arr, selection, &option);
    if (!CHECK_SUCCESS(status) || !option || option->type != JVT_OBJECT) {
        panic(STATUS_INVALID_FORMAT, "an element of array \"options\" is not found or has invalid type");
    }

    fputs("\x1b[3J\x1b[0;0f\x1b[?25h", stdout);

    status = json_object_find_value(&option->obj, "script", &option_script);
    if (!CHECK_SUCCESS(status) || !option_script) {
        panic(STATUS_INVALID_FORMAT, "element \"script\" is not available");
    } else if (option_script->type == JVT_STRING) {
        shell_execute(NULL, option_script->str);
    } else if (option_script->type == JVT_ARRAY) {
        for (struct json_array_elem *elem = option_script->arr.elem; elem; elem = elem->next) {
            if (elem->value->type != JVT_STRING) {
                panic(STATUS_INVALID_FORMAT, "an element of array \"script\" has invalid value type");
            }
            shell_execute(NULL, elem->value->str);
        }
    } else {
        panic(STATUS_INVALID_FORMAT, "element \"script\" has invalid value type");
    }

    for (;;) {
        shell_execute(NULL, "shell");
    }

    json_destruct(json);
}

void main(void)
{
    setup_tty();
    show_menu();

    printf("Kernel returned. Press any key to reboot. (btw, how did you get here?)");
    fgetc(stdin);

    reboot();
}
