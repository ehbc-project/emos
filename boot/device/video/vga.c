#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <mm/mm.h>
#include <device/device.h>
#include <device/driver.h>
#include <interface/framebuffer.h>
#include <interface/console.h>
#include <asm/bios/video.h>
#include <core/panic.h>
#include <asm/vbe/pm_interface.h>

#define DIFF_REGION_SIZE 16

struct vbe_data {
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

static int set_dimension(struct device *dev, int width, int height)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    uint8_t new_mode;
    if (width == 80 && height == 25) {
        new_mode = 0x03;
    } else if (width == 40 && height == 25) {
        new_mode = 0x00;
    } else if (width == 80 && height == 30) {
        new_mode = 0x0C;
    } else {
        return 1;
    }
    int err = _pc_bios_set_video_mode(new_mode);
    if (err) return 1;

    data->video_mode = new_mode;
    data->width = width;
    data->height = height;

    data->char_buffer = mm_reallocate(data->char_buffer, width * height * sizeof(*data->char_buffer));
    memset(data->char_buffer, 0, width * height * sizeof(*data->char_buffer));
    
    data->diff_buffer = mm_reallocate(data->diff_buffer, width * height / 8);
    memset(data->diff_buffer, 0, width * height / 8);

    memset((void *)0xB8000, 0, width * height * sizeof(uint16_t));

    _pc_bios_set_text_cursor_pos(0, 0, 0);

    return 0;
}

static int get_dimension(struct device *dev, int *width, int *height)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    if (data->video_mode == 0xFF) {
        return 1;
    }

    if (width) *width = data->width;
    if (height) *height = data->height;

    return 0;
}

static struct console_char_cell *get_buffer(struct device *dev)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    return data->char_buffer;
}

static void con_invalidate(struct device *dev, int x0, int y0, int x1, int y1)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            data->diff_buffer[(y * data->width + x) / 8] |= 1 << ((y * data->width + x) % 8);
        }
    }
}

static void con_flush(struct device *dev) {}

static void con_present(struct device *dev)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

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
}

static void set_cursor_pos(struct device *dev, int col, int row)
{
    _pc_bios_set_text_cursor_pos(0, row, col);
}

static void get_cursor_pos(struct device *dev, int *col, int *row)
{
    uint8_t b_col, b_row;

    _pc_bios_get_text_cursor(0, NULL, &b_row, &b_col);

    if (col) *col = b_col;
    if (row) *row = b_row;
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

static int probe(struct device *dev);
static int remove(struct device *dev);
static const void *get_interface(struct device *dev, const char *name);

static struct device_driver drv = {
    .name = "vga",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static int probe(struct device *dev)
{
    dev->name = "video";
    dev->id = generate_device_id(dev->name);

    struct vbe_data *data = mm_allocate(sizeof(*data));
    data->char_buffer = NULL;
    data->diff_buffer = NULL;
    data->current_page = 0;
    data->video_mode = 0xFF;
    data->width = 0;
    data->height = 0;

    dev->data = data;

    return 0;
}

static int remove(struct device *dev)
{
    return 0;
}

static const void *get_interface(struct device *dev, const char *name)
{
    if (strcmp(name, "console") == 0) {
        return &conif;
    }

    return NULL;
}

__attribute__((constructor))
static void _register_driver(void)
{
    register_device_driver(&drv);
}

