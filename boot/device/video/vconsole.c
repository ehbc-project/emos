#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>

#include <eboot/asm/io.h>

#include <eboot/status.h>
#include <eboot/macros.h>
#include <eboot/compiler.h>
#include <eboot/font.h>
#include <eboot/device.h>
#include <eboot/interface/framebuffer.h>
#include <eboot/interface/console.h>
#include <eboot/interface/char.h>

struct vconsole_data {
    struct device *fbdev;
    const struct framebuffer_interface *fbif;
    const struct console_interface *conif;

    int passthrough;
    int width, height;
    int cursor_x, cursor_y;
    int cursor_prev_x, cursor_prev_y;
    int cursor_visible;

    struct console_char_cell *char_buffer;
    uint8_t *diff_buffer;
};

inline static int get_glyph_bit(const uint8_t *glyph_data, int cwidth, int x, int y)
{
    return glyph_data[(y * cwidth + x) / 8] & (0x80 >> ((y * cwidth + x) % 8));
}

static status_t draw_char(struct device *dev, int col, int row)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;
    status_t status;
    uint32_t *framebuffer;
    int fb_width;
    struct console_char_cell *cell;
    int cwidth, cheight;
    int x_offset, y_offset;
    uint8_t font_glyph[32];
    static const uint8_t fallback_glyph[16] = {
        0x00, 0x00, 0x00, 0x7E, 0x66, 0x5A, 0x5A, 0x7A,
        0x76, 0x76, 0x7E, 0x76, 0x76, 0x7E, 0x00, 0x00,
    };
    const uint8_t *glyph;
    int use_fallback;

    status = data->fbif->get_framebuffer(data->fbdev, (void **)&framebuffer);
    if (!CHECK_SUCCESS(status)) return status;

    status = data->fbif->get_mode(data->fbdev, &fb_width, NULL, NULL);
    if (!CHECK_SUCCESS(status)) return status;

    cell = &data->char_buffer[row * data->width + col];
    if (cell->codepoint == 0xFFFFFFFF) {
        return STATUS_SUCCESS;
    }

    x_offset = col * 8;
    y_offset = row * 16;

    use_fallback = 0;
    status = font_get_glyph_dimension(cell->codepoint, &cwidth, &cheight);
    if (!CHECK_SUCCESS(status) || (cwidth != 8 && cwidth != 16) || cheight != 16) {
        use_fallback = 1;
    }

    if (!use_fallback) {
        status = font_get_glyph_data(cell->codepoint, font_glyph, sizeof(font_glyph));
        if (!CHECK_SUCCESS(status)) {
            use_fallback = 1;
        }
    }

    if (use_fallback) {
        cwidth = 8;
        cheight = 16;
    }

    glyph = use_fallback ? fallback_glyph : font_glyph;     

    for (int y = 0; y < cheight; y++) {
        for (int x = 0; x < cwidth; x++) {
            int fg = get_glyph_bit(glyph, cwidth, x, y);
            if (cell->attr.text_bold && x > 0) {
                fg |= get_glyph_bit(glyph, cwidth, x - 1, y);
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
            for (int x = 0; x < cwidth; x++) {
                framebuffer[(y_offset + y) * fb_width + x_offset + x] = cell->attr.text_reversed ? cell->attr.bg_color : cell->attr.fg_color;
            }
        }
    }
    
    status = data->fbif->invalidate(data->fbdev, x_offset, y_offset, x_offset + cwidth, y_offset + 16);
    if (!CHECK_SUCCESS(status)) return status;

    return STATUS_SUCCESS;
}

static status_t draw_cursor(struct device *dev)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;
    status_t status;
    int fb_width;
    uint32_t *framebuffer;
    int x_offset, y_offset;

    status = data->fbif->get_mode(data->fbdev, &fb_width, NULL, NULL);
    if (!CHECK_SUCCESS(status)) return status;

    status = data->fbif->get_framebuffer(data->fbdev, (void **)&framebuffer);
    if (!CHECK_SUCCESS(status)) return status;
    
    x_offset = data->cursor_x * 8;
    y_offset = data->cursor_y * 16;

    for (int y = y_offset + 14; y < y_offset + 16; y++) {
        for (int x = x_offset; x < x_offset + 8; x++) {
            framebuffer[y * fb_width + x] = 0xFFFFFF;
        }
    }

    status = data->fbif->invalidate(data->fbdev, x_offset, y_offset + 14, x_offset + 8, y_offset + 16);
    if (!CHECK_SUCCESS(status)) return status;

    return STATUS_SUCCESS;
}

static status_t set_dimension(struct device *dev, int width, int height)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;
    status_t status;

    if (width < 0 && height < 0) {
        status = data->conif->get_dimension(data->fbdev, &width, &height);
        if (!CHECK_SUCCESS(status)) goto has_error;

        data->passthrough = 1;

        if (data->char_buffer) {
            free(data->char_buffer);
            data->char_buffer = NULL;
        }
        if (data->diff_buffer) {
            free(data->diff_buffer);
            data->diff_buffer = NULL;
        }
    } else {
        data->passthrough = 0;
    
        data->char_buffer = realloc(data->char_buffer, width * height * sizeof(*data->char_buffer));
        if (!data->char_buffer) {
            panic(STATUS_UNKNOWN_ERROR, "I think there's no way to recover this");
        }
        memset(data->char_buffer, 0, width * height * sizeof(*data->char_buffer));

        data->diff_buffer = realloc(data->diff_buffer, width * height / 8);
        if (!data->diff_buffer) {
            panic(STATUS_UNKNOWN_ERROR, "I think there's no way to recover this");
        }
        memset(data->diff_buffer, 0, width * height / 8);
    }

    data->width = width;
    data->height = height;

    data->cursor_x = 0;
    data->cursor_y = 0;  

    return STATUS_SUCCESS;

has_error:
    return status;
}

static status_t get_dimension(struct device *dev, int *width, int *height)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        return data->conif->get_dimension(data->fbdev, width, height);
    }

    if (width) *width = data->width;
    if (height) *height = data->height;

    return STATUS_SUCCESS;
}

static status_t get_buffer(struct device *dev, struct console_char_cell **buf)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        return data->conif->get_buffer(data->fbdev, buf);
    }

    if (buf) *buf = data->char_buffer;

    return STATUS_SUCCESS;
}

static status_t invalidate(struct device *dev, int x0, int y0, int x1, int y1)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        return data->conif->invalidate(data->fbdev, x0, y0, x1, y1);
    }

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            data->diff_buffer[(y * data->width + x) / 8] |= 1 << ((y * data->width + x) % 8);
        }
    }

    return STATUS_SUCCESS;
}

static status_t flush(struct device *dev)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;
    status_t status;

    if (data->passthrough) {
        return data->conif->flush(data->fbdev);
    }

    for (int y = 0; y < data->height; y++) {
        for (int x = 0; x < data->width; x++) {
            if (data->diff_buffer[(y * data->width + x) / 8] & (1 << ((y * data->width + x) % 8)) ||
                ((data->cursor_prev_x != data->cursor_x || data->cursor_prev_y != data->cursor_y) &&
                (data->cursor_prev_x == x && data->cursor_prev_y == y))) {
                status = draw_char(dev, x, y);
                // ignore status

                data->diff_buffer[(y * data->width + x) / 8] &= ~(1 << ((y * data->width + x) % 8));
            }
        }
    }

    if (data->cursor_prev_x != data->cursor_x || data->cursor_prev_y != data->cursor_y) {
        data->cursor_prev_x = data->cursor_x;
        data->cursor_prev_y = data->cursor_y;
        
        if (data->cursor_visible) {
            status = draw_cursor(dev);
            // ignore status
        }
    }

    status = data->fbif->flush(data->fbdev);
    if (!CHECK_SUCCESS(status)) return status;

    return STATUS_SUCCESS;
}

static status_t present(struct device *dev)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        return data->conif->present(data->fbdev);
    }

    return data->fbif->present(data->fbdev);
}

static status_t set_cursor_pos(struct device *dev, int col, int row)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        return data->conif->set_cursor_pos(data->fbdev, col, row);
    }

    if (col >= 0) {
        data->cursor_x = col;
    }

    if (row >= 0) {
        data->cursor_y = row;
    }

    return STATUS_SUCCESS;
}

static status_t get_cursor_pos(struct device *dev, int *col, int *row)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        return data->conif->get_cursor_pos(data->fbdev, col, row);
    }

    if (col) *col = data->cursor_x;
    if (row) *row = data->cursor_y;

    return STATUS_SUCCESS;
}

static status_t set_cursor_visibility(struct device *dev, int visibility)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        return data->conif->set_cursor_visibility(dev, visibility);
    }

    data->cursor_visible = visibility;

    return STATUS_SUCCESS;
}

static status_t get_cursor_visibility(struct device *dev, int *visibility)
{
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

    if (data->passthrough) {
        return data->conif->get_cursor_visibility(dev, visibility);
    }

    if (visibility) *visibility = data->cursor_visible;

    return STATUS_SUCCESS;
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

static status_t probe(struct device **devout, struct device_driver *drv, struct device *parent, struct resource *rsrc, int rsrc_cnt);
static status_t remove(struct device *dev);
static status_t get_interface(struct device *dev, const char *name, const void **result);

static void vconsole_init(void)
{
    status_t status;
    struct device_driver *drv;

    status = device_driver_create(&drv);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "cannot register device driver \"vconsole\"");
    }

    drv->name = "vconsole";
    drv->probe = probe;
    drv->remove = remove;
    drv->get_interface = get_interface;
}

static status_t probe(struct device **devout, struct device_driver *drv, struct device *parent, struct resource *rsrc, int rsrc_cnt)
{
    status_t status;
    struct device *dev = NULL;
    struct device *fbdev = NULL;
    const struct framebuffer_interface *fbif = NULL;
    const struct console_interface *conif = NULL;
    struct vconsole_data *data = NULL;

    fbdev = parent;
    if (!fbdev) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }
    
    status = fbdev->driver->get_interface(fbdev, "framebuffer", (const void **)&fbif);
    if (!CHECK_SUCCESS(status)) goto has_error;
    
    status = fbdev->driver->get_interface(fbdev, "console", (const void **)&conif);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = device_create(&dev, drv, parent);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = device_generate_name("console", dev->name, sizeof(dev->name));
    if (!CHECK_SUCCESS(status)) goto has_error;

    data = malloc(sizeof(*data));
    data->fbdev = fbdev;
    data->fbif = fbif;
    data->conif = conif;
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
    struct vconsole_data *data = (struct vconsole_data *)dev->data;

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

DEVICE_DRIVER(vconsole, vconsole_init)
