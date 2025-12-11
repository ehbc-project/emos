#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <eboot/asm/bios/video.h>

#include <eboot/status.h>
#include <eboot/macros.h>
#include <eboot/panic.h>
#include <eboot/device.h>
#include <eboot/interface/framebuffer.h>
#include <eboot/interface/console.h>

#define DIFF_REGION_SIZE 16

struct vga_data {
    struct console_char_cell *char_buffer;
    uint8_t *diff_buffer;
    int current_page;

    uint8_t video_mode;
    int width, height;
};

static uint8_t rgb_to_irgb(uint32_t color)
{
    uint8_t r = (color >> 16) & 0xFF, g = (color >> 8) & 0xFF, b = color & 0xFF;

    uint8_t max = r;
    uint8_t min = r;
    if (g > max) max = g;
    if (g < min) min = g;
    if (b > max) max = b;
    if (b < min) min = b;
    
    uint8_t max_diff = max - min;
    if (max_diff < 0x50) {
        if ((max + min) / 2 < 0x40) {
            return 0x00;
        } else if ((max + min) / 2 <= 0x80) {
            return 0x08;
        } else if ((max + min) / 2 < 0xC0) {
            return 0x07;
        } else {
            return 0x0F;
        }
    }
    
    return (((max >= 192) ? 1 : 0) << 3) | ((r >= 0x80 ? 1 : 0) << 2) | ((g >= 0x80 ? 1 : 0) << 1) | (b >= 0x80 ? 1 : 0);
}

static status_t set_dimension(struct device *dev, int width, int height)
{
    struct vga_data *data = (struct vga_data*)dev->data;
    status_t status;
    uint8_t new_mode = 0xFF;

    if (width == 80 && height == 25) {
        new_mode = 0x03;
    } else if (width == 40 && height == 25) {
        new_mode = 0x00;
    } else if (width == 80 && height == 30) {
        new_mode = 0x0C;
    } else {
        status = STATUS_ENTRY_NOT_FOUND;
        goto has_error;
    }

    _pc_bios_video_set_mode(new_mode);

    data->video_mode = new_mode;
    data->width = width;
    data->height = height;

    data->char_buffer = realloc(data->char_buffer, width * height * sizeof(*data->char_buffer));
    if (!data->diff_buffer) {
        status = STATUS_UNKNOWN_ERROR;
        goto has_error;
    }
    memset(data->char_buffer, 0, width * height * sizeof(*data->char_buffer));
    
    data->diff_buffer = realloc(data->diff_buffer, width * height / 8);
    if (!data->diff_buffer) {
        status = STATUS_UNKNOWN_ERROR;
        goto has_error;
    }
    memset(data->diff_buffer, 0, width * height / 8);

    memset((void *)0xB8000, 0, width * height * sizeof(uint16_t));

    _pc_bios_video_set_cursor_pos(0, 0, 0);

    return STATUS_SUCCESS;

has_error:
    if (new_mode != 0xFF) {
        panic(status, "I don't know what more can I do (vga, set_dimension())");
    }

    return status;
}

static status_t get_dimension(struct device *dev, int *width, int *height)
{
    struct vga_data *data = (struct vga_data*)dev->data;

    if (data->video_mode == 0xFF) {
        return 1;
    }

    if (width) *width = data->width;
    if (height) *height = data->height;

    return STATUS_SUCCESS;
}

static status_t get_buffer(struct device *dev, struct console_char_cell **buf)
{
    struct vga_data *data = (struct vga_data*)dev->data;

    if (buf) *buf = data->char_buffer;

    return STATUS_SUCCESS;
}

static status_t con_invalidate(struct device *dev, int x0, int y0, int x1, int y1)
{
    struct vga_data *data = (struct vga_data*)dev->data;

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            data->diff_buffer[(y * data->width + x) / 8] |= 1 << ((y * data->width + x) % 8);
        }
    }

    return STATUS_SUCCESS;
}

static status_t con_flush(struct device *dev) {
    return STATUS_SUCCESS;
}

static status_t con_present(struct device *dev)
{
    struct vga_data *data = (struct vga_data*)dev->data;

    for (int y = 0; y < data->height; y++) {
        for (int x = 0; x < data->width; x++) {
            if (data->diff_buffer[(y * data->width + x) / 8] & (1 << ((y * data->width + x) % 8))) {
                const struct console_char_cell *src = &data->char_buffer[y * data->width + x];
                uint16_t cell = src->codepoint & 0xFF;
                cell |= (rgb_to_irgb(src->attr.fg_color) & 0xF) << 8;
                cell |= (rgb_to_irgb(src->attr.bg_color) & 0xF) << 12;
                if (src->attr.text_dim) {
                    cell &= 0x77FF;
                }
                ((uint16_t *)0xB8000)[y * data->width + x] = cell;
                data->diff_buffer[(y * data->width + x) / 8] &= ~(1 << ((y * data->width + x) % 8));
            }
        }
    }

    return STATUS_SUCCESS;
}

static status_t set_cursor_pos(struct device *dev, int col, int row)
{
    _pc_bios_video_set_cursor_pos(0, row, col);

    return STATUS_SUCCESS;
}

static status_t get_cursor_pos(struct device *dev, int *col, int *row)
{
    uint8_t b_col, b_row;

    _pc_bios_video_get_cursor_info(0, NULL, &b_row, &b_col);

    if (col) *col = b_col;
    if (row) *row = b_row;

    return STATUS_SUCCESS;
}

static const struct console_interface conif = {
    .set_dimension = set_dimension,
    .get_dimension = get_dimension,
    .get_buffer = get_buffer,
    .invalidate = con_invalidate,
    .flush = con_flush,
    .present = con_present,
    .set_cursor_pos = set_cursor_pos,
    .get_cursor_pos = get_cursor_pos,
    .set_cursor_attr = NULL,
    .get_cursor_attr = NULL,
};

static status_t probe(struct device **devout, struct device_driver *drv, struct device *parent, struct resource *rsrc, int rsrc_cnt);
static status_t remove(struct device *dev);
static status_t get_interface(struct device *dev, const char *name, const void **result);

static void vga_init(void)
{
    status_t status;
    struct device_driver *drv;

    status = device_driver_create(&drv);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "cannot register device driver \"vga\"");
    }

    drv->name = "vga";
    drv->probe = probe;
    drv->remove = remove;
    drv->get_interface = get_interface;
}

static status_t probe(struct device **devout, struct device_driver *drv, struct device *parent, struct resource *rsrc, int rsrc_cnt)
{
    status_t status;
    struct device *dev = NULL;
    struct vga_data *data = NULL;

    status = device_create(&dev, drv, parent);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = device_generate_name("video", dev->name, sizeof(dev->name));
    if (!CHECK_SUCCESS(status)) goto has_error;

    data = malloc(sizeof(*data));
    data->char_buffer = NULL;
    data->diff_buffer = NULL;
    data->current_page = 0;
    data->video_mode = 0xFF;
    data->width = 0;
    data->height = 0;
    dev->data = data;

    status = set_dimension(dev, 80, 25);
    
    if (devout) *devout = dev;

    return STATUS_SUCCESS;

has_error:
    if (data) {
        free(data);
    }

    if (dev) {
        device_remove(dev);
    }

    return status;
}

static status_t remove(struct device *dev)
{
    struct vga_data *data = (struct vga_data *)dev->data;

    if (data->char_buffer) {
        free(data->char_buffer);
    }

    if (data->diff_buffer) {
        free(data->diff_buffer);
    }

    free(data);

    device_remove(dev);

    return STATUS_SUCCESS;
}

static status_t get_interface(struct device *dev, const char *name, const void **result)
{
    if (strcmp(name, "console") == 0) {
        if (result) *result = &conif;
        return STATUS_SUCCESS;
    }

    return STATUS_ENTRY_NOT_FOUND;
}

DEVICE_DRIVER(vga, vga_init)
