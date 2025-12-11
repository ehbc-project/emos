#include <eboot/shell.h>

#include <stdio.h>
#include <stdlib.h>

#include <getopt.h>

#include <eboot/asm/bios/video.h>

#include <eboot/status.h>
#include <eboot/device.h>
#include <eboot/interface/framebuffer.h>
#include <eboot/interface/console.h>

static const struct option opts[] = {
    { "list", optional_argument, 0, 'l' },
    { NULL, 0, NULL, 0 },
};

static int list_modes(char *argv0)
{
    status_t status;
    struct vbe_controller_info vbe_info;
    uint16_t *mode_list;
    struct vbe_video_mode_info vbe_mode_info;

    status = _pc_bios_vbe_get_controller_info(&vbe_info);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "%s: could not list video modes\n", argv0);
        return 1;
    }

    mode_list = (uint16_t *)((vbe_info.video_modes.segment << 4) + vbe_info.video_modes.offset);

    printf("Video Modes:\r\n");
    for (int i = 0; mode_list[i] != 0xFFFF; i++) {
        status = _pc_bios_vbe_get_video_mode_info(mode_list[i], &vbe_mode_info);
        if (!CHECK_SUCCESS(status)) {
            fprintf(stderr, "%s: could not get video mode info\n", argv0);
            return 1;
        }
        if (vbe_mode_info.memory_model == VBEMM_TEXT) {
            printf("t%dx%d\t",
                vbe_mode_info.width,
                vbe_mode_info.height
            );
        } else if (vbe_mode_info.memory_model == VBEMM_DIRECT) {
            printf("%dx%dx%d\t",
                vbe_mode_info.width,
                vbe_mode_info.height,
                vbe_mode_info.bpp
            );
        }
    }
    fputs("\n", stdout);

    return 0;
}

static int vmode_handler(struct shell_instance *inst, int argc, char **argv)
{
    status_t status;
    int opt, list = 0;
    struct device *fbdev;
    struct device *condev;
    const struct console_interface *conif;
    char *heightstr, *bppstr;
    int width, height, bpp;
    const struct console_interface *fbconif;
    const struct framebuffer_interface *fbif;

    if (argc < 2) {
        printf("usage: %s [-l] [mode]\n", argv[0]);
        return 1;
    }

    getopt_init();
    while ((opt = getopt_long(argc, argv, "l", opts, NULL)) != -1) {
        switch (opt) {
            case 'l':
                list = 1;
                break;
            default:
                printf("usage: %s [-l] [mode]\n", argv[0]);
                return 1;
        }
    }

    if (list) {
        return list_modes(argv[0]);
    }

    status = device_find("video0", &fbdev);
    if (!CHECK_SUCCESS(status)) return 1;

    status = device_find("console0", &condev);
    if (!CHECK_SUCCESS(status)) return 1;

    status = condev->driver->get_interface(condev, "console", (const void **)&conif);
    if (!CHECK_SUCCESS(status)) return 1;

    if (argv[1][0] == 't') {
        width = strtol(argv[1] + 1, &heightstr, 10);
        if (!heightstr || !heightstr[0]) {
            fprintf(stderr, "%s: invalid video mode\n", argv[0]);
            return 1;
        }
        height = strtol(heightstr + 1, NULL, 10);

        status = fbdev->driver->get_interface(fbdev, "console", (const void **)&fbconif);
        if (!CHECK_SUCCESS(status)) return 1;

        status = fbconif->set_dimension(fbdev, width, height);
        if (!CHECK_SUCCESS(status)) {
            fprintf(stderr, "%s: could not switch video mode\n", argv[0]);
            return 1;
        }

        status = conif->set_dimension(condev, -1, -1);
        if (!CHECK_SUCCESS(status)) {
            fprintf(stderr, "%s: could not set dimension\n", argv[0]);
            return 1;
        }
    } else {
        width = strtol(argv[1], &heightstr, 10);
        if (!heightstr || !heightstr[0]) {
            fprintf(stderr, "%s: invalid video mode\n", argv[0]);
            return 1;
        }
        height = strtol(heightstr + 1, &bppstr, 10);
        if (!bppstr || !bppstr[0]) {
            fprintf(stderr, "%s: invalid video mode\n", argv[0]);
            return 1;
        }
        bpp = strtol(bppstr + 1, NULL, 10);

        status = fbdev->driver->get_interface(fbdev, "framebuffer", (const void **)&fbif);
        if (!CHECK_SUCCESS(status)) return 1;

        status = fbif->set_mode(fbdev, width, height, bpp);
        if (!CHECK_SUCCESS(status)) {
            fprintf(stderr, "%s: could not switch video mode\n", argv[0]);
            return 1;
        }

        status = conif->set_dimension(condev, width / 8, height / 16);
        if (!CHECK_SUCCESS(status)) {
            fprintf(stderr, "%s: could not set console dimension\n", argv[0]);
            return 1;
        }
    }

    return 0;
}

static struct command vmode_command = {
    .name = "vmode",
    .handler = vmode_handler,
    .help_message = "List or set video mode",
};

static void vmode_command_init(void)
{
    shell_command_register(&vmode_command);
}

SHELL_COMMAND(vmode, vmode_command_init)
