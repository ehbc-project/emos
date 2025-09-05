#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <wchar.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <interface/framebuffer.h>
#include <interface/console.h>
#include <interface/char.h>
#include <asm/io.h>

struct vconsole_data {
    struct device *fbdev;
    const struct framebuffer_interface *fbdev_fbif;
    const struct console_interface *fbdev_conif;

    int passthrough;
    int width, height;
    int cursor_x, cursor_y;
    int cursor_prev_x, cursor_prev_y;
    int cursor_visible;

    struct console_char_cell *char_buffer;
    uint8_t *diff_buffer;
};

extern int font_is_glyph_full_width(uint32_t codepoint);
extern int font_get_glyph(uint32_t codepoint, uint8_t *buf, long size);

static int get_glyph_bit(uint8_t *glyph_data, int is_full_width, int x, int y)
{
    uint8_t current_byte = glyph_data[y * (is_full_width ? 2 : 1) + x / 8];
    return current_byte & (0x80 >> (x % 8));
}

static void draw_char(struct device *dev, int col, int row)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    uint32_t *framebuffer = data->fbdev_fbif->get_framebuffer(data->fbdev);
    int fb_width;
    data->fbdev_fbif->get_mode(data->fbdev, &fb_width, NULL, NULL);

    struct console_char_cell *cell = &data->char_buffer[row * data->width + col];
    if (cell->codepoint == 0xFFFFFFFF) return;
    int is_full_width = font_is_glyph_full_width(cell->codepoint);
    uint8_t glyph[32];
    int x_offset = col * 8, y_offset = row * 16;

    int err = font_get_glyph(cell->codepoint, glyph, sizeof(glyph));
    if (err) return;

    for (int y = 0; y < 16; y++) {
        for (int x = 0; x < (is_full_width ? 16 : 8); x++) {
            int fg = get_glyph_bit(glyph, is_full_width, x, y);
            if (cell->attr.text_bold && x > 0) {
                fg |= get_glyph_bit(glyph, is_full_width, x - 1, y);
            }

            uint32_t fg_color = cell->attr.fg_color;
            uint32_t bg_color = cell->attr.bg_color;

            if (cell->attr.text_dim) {
                fg_color &= 0xFCFCFC;
                fg_color = (fg_color >> 1) + (fg_color >> 2);
                bg_color &= 0xFCFCFC;
                bg_color = (bg_color >> 1) + (bg_color >> 2);
            }

            if (cell->attr.text_reversed) {
                framebuffer[(y_offset + y) * fb_width + x_offset + x] = fg ? bg_color : fg_color;
            } else {
                framebuffer[(y_offset + y) * fb_width + x_offset + x] = fg ? fg_color : bg_color;
            }
        }

        if ((cell->attr.text_overlined && y == 1) ||
            (cell->attr.text_strike && y == 8) ||
            (cell->attr.text_underline && y == 15)) {
            for (int x = 0; x < (is_full_width ? 16 : 8); x++) {
                framebuffer[(y_offset + y) * fb_width +  x_offset + x] = cell->attr.text_reversed ? cell->attr.bg_color : cell->attr.fg_color;
            }
        }
    }
    
    data->fbdev_fbif->invalidate(data->fbdev, x_offset, y_offset, x_offset + (is_full_width ? 16 : 8), y_offset + 16);
}

static void draw_cursor(struct device *dev)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    int fb_width;
    data->fbdev_fbif->get_mode(data->fbdev, &fb_width, NULL, NULL);
    uint32_t *framebuffer = data->fbdev_fbif->get_framebuffer(data->fbdev);
    int x_offset = data->cursor_x * 8;
    int y_offset = data->cursor_y * 16;

    for (int y = y_offset + 14; y < y_offset + 16; y++) {
        for (int x = x_offset; x < x_offset + 8; x++) {
            framebuffer[y * fb_width + x] = 0xFFFFFF;
        }
    }

    data->fbdev_fbif->invalidate(data->fbdev, x_offset, y_offset + 14, x_offset + 8, y_offset + 16);
}

static int set_dimension(struct device *dev, int width, int height)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (width < 0 && height < 0) {
        if (!data->fbdev_conif) {
            return 1;
        }

        if (data->fbdev_conif->get_dimension(data->fbdev, &width, &height)) {
            return 1;
        }

        data->passthrough = 1;

        if (data->char_buffer) {
            mm_free(data->char_buffer);
            data->char_buffer = NULL;
        }
        if (data->diff_buffer) {
            mm_free(data->diff_buffer);
            data->diff_buffer = NULL;
        }
    } else {
        data->passthrough = 0;
    
        data->char_buffer = mm_reallocate(data->char_buffer, width * height * sizeof(*data->char_buffer));
        memset(data->char_buffer, 0, width * height * sizeof(*data->char_buffer));
        data->diff_buffer = mm_reallocate(data->diff_buffer, width * height / 8);
        memset(data->diff_buffer, 0, width * height / 8);
    }

    data->width = width;
    data->height = height;

    data->cursor_x = 0;
    data->cursor_y = 0;  

    return 0;
}

static int get_dimension(struct device *dev, int *width, int *height)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        return data->fbdev_conif->get_dimension(data->fbdev, width, height);
    }

    if (width) *width = data->width;
    if (height) *height = data->height;

    return 0;
}

static struct console_char_cell *get_buffer(struct device *dev)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        return data->fbdev_conif->get_buffer(data->fbdev);
    }

    return data->char_buffer;
}

static void invalidate(struct device *dev, int x0, int y0, int x1, int y1)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        data->fbdev_conif->invalidate(data->fbdev, x0, y0, x1, y1);
        return;
    }

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            data->diff_buffer[(y * data->width + x) / 8] |= 1 << ((y * data->width + x) % 8);
        }
    }
}

static void flush(struct device *dev)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        data->fbdev_conif->flush(data->fbdev);
        return;
    }

    for (int y = 0; y < data->height; y++) {
        for (int x = 0; x < data->width; x++) {
            if (data->diff_buffer[(y * data->width + x) / 8] & (1 << ((y * data->width + x) % 8)) ||
                ((data->cursor_prev_x != data->cursor_x || data->cursor_prev_y != data->cursor_y) &&
                (data->cursor_prev_x == x && data->cursor_prev_y == y))) {
                draw_char(dev, x, y);
                data->diff_buffer[(y * data->width + x) / 8] &= ~(1 << ((y * data->width + x) % 8));
            }
        }
    }

    if (data->cursor_prev_x != data->cursor_x || data->cursor_prev_y != data->cursor_y) {
        data->cursor_prev_x = data->cursor_x;
        data->cursor_prev_y = data->cursor_y;
        if (data->cursor_visible) {
            draw_cursor(dev);
        }
    }

    data->fbdev_fbif->flush(data->fbdev);
}

static void present(struct device *dev)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        data->fbdev_conif->present(data->fbdev);
        return;
    }

    data->fbdev_fbif->present(data->fbdev);
}

static void set_cursor_pos(struct device *dev, int col, int row)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        data->fbdev_conif->set_cursor_pos(data->fbdev, col, row);
        return;
    }

    if (col >= 0) {
        data->cursor_x = col;
    }

    if (row >= 0) {
        data->cursor_y = row;
    }
}

static void get_cursor_pos(struct device *dev, int *col, int *row)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        data->fbdev_conif->get_cursor_pos(data->fbdev, col, row);
        return;
    }

    if (col) *col = data->cursor_x;
    if (row) *row = data->cursor_y;
}

static void set_cursor_visibility(struct device *dev, int visible)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    data->cursor_visible = visible;
}

static int get_cursor_visibility(struct device *dev)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    return data->cursor_visible;
}

static const struct console_interface conif = {
    .set_dimension = set_dimension,
    .get_dimension = get_dimension,
    .get_buffer = get_buffer,
    .invalidate = invalidate,
    .flush = flush,
    .present = present,
    .set_cursor_pos = set_cursor_pos,
    .get_cursor_pos = get_cursor_pos,
    .set_cursor_visibility = set_cursor_visibility,
    .get_cursor_visibility = get_cursor_visibility,
    .set_cursor_attr = NULL,
    .get_cursor_attr = NULL,
};

static int probe(struct device *dev);
static int remove(struct device *dev);
static const void *get_interface(struct device *dev, const char *name);

static struct device_driver drv = {
    .name = "vconsole",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static int probe(struct device *dev)
{
    dev->name = "console";
    dev->id = generate_device_id(dev->name);

    struct device *fbdev = dev->parent;
    if (!fbdev) return 1;
    const struct framebuffer_interface *fbdev_fbif = fbdev->driver->get_interface(fbdev, "framebuffer");
    if (!fbdev_fbif) return 1;
    const struct console_interface *fbdev_conif = fbdev->driver->get_interface(fbdev, "console");

    struct vconsole_data *data = mm_allocate(sizeof(*data));
    data->fbdev = fbdev;
    data->fbdev_fbif = fbdev_fbif;
    data->fbdev_conif = fbdev_conif;
    data->char_buffer = NULL;
    data->diff_buffer = NULL;
    data->width = 0;
    data->height = 0;
    data->cursor_prev_x = -1;
    data->cursor_prev_y = -1;
    data->cursor_x = 0;
    data->cursor_y = 0;
    data->cursor_visible = 1;
    data->passthrough = 0;

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
