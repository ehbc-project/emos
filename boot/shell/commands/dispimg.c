#include <eboot/shell.h>

#include <stdio.h>
#include <string.h>

#include <eboot/status.h>
#include <eboot/path.h>
#include <eboot/device.h>
#include <eboot/interface/framebuffer.h>

static int dispimg_handler(struct shell_instance *inst, int argc, char **argv)
{
    status_t status;

    if (argc < 2) {
        fprintf(stderr, "usage: %s path\n", argv[0]);
        return 1;
    }

    char path[PATH_MAX];
    if (path_is_absolute(argv[1])) {
        strncpy(path, argv[1], sizeof(path) - 1);
    } else {
        strncpy(path, inst->working_dir_path, sizeof(path) - 1);
        path_join(path, sizeof(path), argv[1]);

        if (!inst->fs) {
            fprintf(stderr, "%s: filesystem not selected\n", argv[0]);
            return 1;
        }
    }

    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "%s: failed to open file\n", argv[0]);
        return 1;
    }

    struct device *fbdev;
    status = device_find("video0", &fbdev);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "%s: cannot find device\n", argv[0]);
        return 1;
    }

    const struct framebuffer_interface *fbif;
    status = fbdev->driver->get_interface(fbdev, "framebuffer", (const void **)&fbif);
    if (!CHECK_SUCCESS(status)) return 1;

    int width, height;
    uint32_t *framebuffer;
    status = fbif->get_framebuffer(fbdev, (void **)&framebuffer);
    if (!CHECK_SUCCESS(status)) return 1;

    status = fbif->get_mode(fbdev, &width, &height, NULL);
    if (!CHECK_SUCCESS(status)) return 1;

    struct bmp_header {
        char signature[2];
        uint32_t file_size;
        uint8_t unused[4];
        uint32_t bitmap_offset;
    } __packed;

    struct bmp_dibheader_core {
        uint32_t header_size;
        uint32_t width;
        uint32_t height;
        uint16_t color_planes;
        uint16_t bpp;
    } __packed;

    struct bmp_header header;
    fread(&header, sizeof(header), 1, fp);

    if (header.signature[0] != 'B' || header.signature[1] != 'M') {
        fprintf(stderr, "%s: not a BMP file\n", argv[0]);
        fclose(fp);
        return 1;
    }

    struct bmp_dibheader_core dibheader;
    fread(&dibheader, sizeof(dibheader), 1, fp);

    printf("bmp file size: %lu bytes, bitmap offset %lu\n", header.file_size, header.bitmap_offset);
    printf("width: %lu, height: %lu, bpp: %u\n", dibheader.width, dibheader.height, dibheader.bpp);

    fseek(fp, header.bitmap_offset, SEEK_SET);

    int ystart = (height - dibheader.height) / 2;
    int xstart = (width - dibheader.width) / 2;

    for (int y = 0; y < dibheader.height; y++) {
        for (int x = 0; x < dibheader.width; x++) {
            uint32_t buf;
            fread(&buf, dibheader.bpp / 8, 1, fp);
            framebuffer[(ystart + dibheader.height - y - 1) * width + xstart + x] = buf;
        }
    }
    fbif->invalidate(fbdev, 0, 0, width - 1, height - 1);
    fbif->flush(fbdev);
    fbif->present(fbdev);
    
    fclose(fp);

    char ch;
    fread(&ch, sizeof(ch), 1, stdin);

    fputs("\x1b[3J\x1b[0;0f", stdout);

    return 0;
}

static struct command dispimg_command = {
    .name = "dispimg",
    .handler = dispimg_handler,
    .help_message = "Write arguments to a device",
};

static void dispimg_command_init(void)
{
    shell_command_register(&dispimg_command);
}

SHELL_COMMAND(dispimg, dispimg_command_init)
