#include <device/video/vconsole.h>

#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <interface/console.h>
#include <interface/char.h>

extern int font_is_glyph_full_width(uint32_t codepoint);
extern int font_get_glyph(uint32_t codepoint, uint8_t *buf, long size);

static int get_glyph_bit(uint8_t *glyph_data, int is_full_width, int x, int y)
{
    uint8_t current_byte = glyph_data[y * (is_full_width ? 2 : 1) + x / 8];
    return current_byte & (0x80 >> (x % 8));
}

static void draw_char(struct vconsole_device *dev, int col, int row)
{
    uint32_t *framebuffer = dev->fbdev_fbif->get_framebuffer(dev->fbdev);
    int fb_width;
    dev->fbdev_fbif->get_mode(dev->fbdev, &fb_width, NULL, NULL);

    struct console_char_cell *cell = &dev->char_buffer[row * dev->width + col];
    int is_full_width = font_is_glyph_full_width(cell->codepoint);
    uint8_t glyph[32];
    int x_offset = col * 8, y_offset = row * 16;

    if (font_get_glyph(cell->codepoint, glyph, 32)) return;

    for (int y = y_offset; y < y_offset + 16; y++) {
        for (int x = x_offset; x < x_offset + (is_full_width ? 16 : 8); x++) {
            int fg = get_glyph_bit(glyph, is_full_width, x - x_offset, y - y_offset);
            if (cell->attr.text_bold && x > x_offset) {
                fg |= get_glyph_bit(glyph, is_full_width, x - x_offset - 1, y - y_offset);
            }

            if (cell->attr.text_reversed) {
                framebuffer[y * fb_width + x] = fg ? cell->attr.bg_color : cell->attr.fg_color;
            } else {
                framebuffer[y * fb_width + x] = fg ? cell->attr.fg_color : cell->attr.bg_color;
            }
        }

        if ((cell->attr.text_overlined && y == y_offset + 1) ||
            (cell->attr.text_strike && y == y_offset + 8) ||
            (cell->attr.text_underline && y == y_offset + 15)) {
            for (int x = x_offset; x < x_offset + (is_full_width ? 16 : 8); x++) {
                framebuffer[y * fb_width + x] = cell->attr.text_reversed ? cell->attr.bg_color : cell->attr.fg_color;
            }
        }
    }
    
    dev->fbdev_fbif->invalidate(dev->fbdev, x_offset, y_offset, x_offset + (is_full_width ? 16 : 8), y_offset + 16);
}

static void set_dimension(struct device *_dev, int width, int height)
{
    struct vconsole_device *dev = (struct vconsole_device *)_dev;

    dev->width = width;
    dev->height = height;

    dev->char_buffer = mm_reallocate(dev->char_buffer, width * height * sizeof(struct console_char_cell));
    dev->diff_buffer = mm_reallocate(dev->diff_buffer, width * height / 8);

    dev->cursor_x = 0;
    dev->cursor_y = 0;
}

static void get_dimension(struct device *_dev, int *width, int *height)
{
    struct vconsole_device *dev = (struct vconsole_device *)_dev;

    if (width) *width = dev->width;
    if (height) *height = dev->height;
}

static struct console_char_cell *get_buffer(struct device *_dev)
{
    struct vconsole_device *dev = (struct vconsole_device *)_dev;

    return dev->char_buffer;
}

static void invalidate(struct device *_dev, int x0, int y0, int x1, int y1)
{
    struct vconsole_device *dev = (struct vconsole_device *)_dev;

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            dev->diff_buffer[(y * dev->width + x) / 8] |= 1 << ((y * dev->width + x) % 8);
        }
    }
}

static void flush(struct device *_dev)
{
    struct vconsole_device *dev = (struct vconsole_device *)_dev;

    for (int y = 0; y < dev->height; y++) {
        for (int x = 0; x < dev->width; x++) {
            if (dev->diff_buffer[(y * dev->width + x) / 8] & (1 << ((y * dev->width + x) % 8))) {
                draw_char(dev, x, y);
                dev->diff_buffer[(y * dev->width + x) / 8] &= ~(1 << ((y * dev->width + x) % 8));
            }
        }
    }

    dev->fbdev_fbif->flush(dev->fbdev);
    dev->fbdev_fbif->present(dev->fbdev);
}

static void present(struct device *_dev)
{
    struct vconsole_device *dev = (struct vconsole_device *)_dev;
    
    dev->fbdev_fbif->present(_dev);
}

static void set_cursor_pos(struct device *_dev, int col, int row)
{
    struct vconsole_device *dev = (struct vconsole_device *)_dev;

    dev->cursor_x = col;
    dev->cursor_y = row;
}

static void get_cursor_pos(struct device *_dev, int *col, int *row)
{
    struct vconsole_device *dev = (struct vconsole_device *)_dev;

    if (col) *col = dev->cursor_x;
    if (row) *row = dev->cursor_y;
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
    .set_cursor_attr = NULL,
    .get_cursor_attr = NULL,
};

static struct device *probe(const struct device_id *id);
static int remove(struct device *_dev);
static const void *get_interface(struct device *_dev, const char *name);

static struct device_driver drv = {
    .name = "vconsole",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static struct device *probe(const struct device_id *id)
{
    struct vconsole_device *dev = mm_allocate(sizeof(*dev));
    if (!dev) return NULL;
    dev->dev.driver = &drv;

    struct device_id *current_id = mm_allocate(sizeof(*current_id));
    current_id->type = DIT_STRING;
    current_id->string = "chrvideo";
    dev->dev.id = current_id;

    if (id->type != DIT_STRING) return NULL;
    struct device *fbdev = find_device(id->string);
    if (!fbdev) return NULL;
    dev->fbdev = fbdev;

    const struct framebuffer_interface *fbdev_fbif = fbdev->driver->get_interface(fbdev, "framebuffer");
    if (!fbdev_fbif) return NULL;
    dev->fbdev_fbif = fbdev_fbif;

    dev->conif = &conif;

    dev->char_buffer = NULL;
    dev->diff_buffer = NULL;

    dev->width = 0;
    dev->height = 0;
    dev->cursor_x = 0;
    dev->cursor_y = 0;

    register_device((struct device *)dev);

    return (struct device *)dev;
}

static int remove(struct device *_dev)
{
    struct vconsole_device *dev = (struct vconsole_device *)_dev;

    mm_free(dev);

    return 0;
}

static const void *get_interface(struct device *_dev, const char *name)
{
    struct vconsole_device *dev = (struct vconsole_device *)_dev;

    if (strcmp(name, "console") == 0) {
        return dev->conif;
    }

    return NULL;
}

__attribute__((constructor))
static void _register_driver(void)
{
    register_driver((struct device_driver *)&drv);
}
