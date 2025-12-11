#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <eboot/asm/bios/video.h>
#include <eboot/asm/bios/vbe_pmi.h>

#include <eboot/status.h>
#include <eboot/macros.h>
#include <eboot/panic.h>
#include <eboot/mm.h>
#include <eboot/device.h>
#include <eboot/encoding/cp437.h>
#include <eboot/interface/framebuffer.h>
#include <eboot/interface/console.h>

#define DIFF_REGION_SIZE 16

struct vbe_data {
    struct console_char_cell *char_buffer;
    uint32_t *frame_buffer;
    uint8_t *diff_buffer;
    int current_page;

    farptr_t pmi_table;

    struct vbe_video_mode_info mode_info;
    size_t hw_framebuffer_size;

    void (*convert_color)(const struct vbe_video_mode_info *, const uint32_t *, void *, long);
};

inline static uint32_t get_pixel(struct device *dev, int x, int y)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    return data->frame_buffer[y * data->mode_info.width + x];
}

inline static void set_region_diff(struct device *dev, int xregion, int yregion)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    data->diff_buffer[yregion * data->mode_info.width / DIFF_REGION_SIZE + xregion] = 1;
}

inline static void reset_region_diff(struct device *dev, int xregion, int yregion)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    data->diff_buffer[yregion * data->mode_info.width / DIFF_REGION_SIZE  + xregion] = 0;
}

inline static int get_region_diff(struct device *dev, int xregion, int yregion)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    return data->diff_buffer[yregion * data->mode_info.width / DIFF_REGION_SIZE  + xregion];
}

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

__attribute__((hot))
static void convert_color_rgbx8888(const struct vbe_video_mode_info *mode, const uint32_t *orig, void *dest, long count)
{
    memcpy(dest, orig, count * sizeof(uint32_t));
}

__attribute__((hot))
static void convert_color_rgb888(const struct vbe_video_mode_info *mode, const uint32_t *orig, void *dest, long count)
{
    while (count > 0) {
        if ((uint32_t)dest & 1) {
            *(uint8_t *)dest = *(const uint8_t *)orig;
            *(uint16_t *)((uint8_t *)dest + 1) = *(const uint16_t *)((const uint8_t *)orig + 1);
        } else if (count > 1) {
            *(uint32_t *)dest = *orig;
        } else {
            *(uint16_t *)dest = *(const uint16_t *)orig;
            *((uint8_t *)dest + 2) = *((const uint8_t *)orig + 2);
        }

        dest = (uint8_t *)dest + 3;
        orig++;
        count--;
    }
}

__attribute__((hot))
static void convert_color_rgb565(const struct vbe_video_mode_info *mode, const uint32_t *orig, void *dest, long count)
{
    while (count > 0) {
        *(uint16_t *)dest = ((*orig & 0xF80000) >> 8) | ((*orig & 0x00FC00) >> 5) | ((*orig & 0x0000F8) >> 3);

        dest = (uint16_t *)dest + 1;
        orig++;
        count--;
    }
}

__attribute__((hot))
static void convert_color_generic(const struct vbe_video_mode_info *mode, const uint32_t *orig, void *dest, long count)
{
    while (count > 0) {
        uint8_t r = ((*orig >> 16) & 0xFF) >> (8 - mode->red_mask);
        uint8_t g = ((*orig >> 8) & 0xFF) >> (8 - mode->green_mask);
        uint8_t b = (*orig & 0xFF) >> (8 - mode->blue_mask);

        uint32_t color = 
            (r << mode->red_position) |
            (g << mode->green_position) |
            (b << mode->blue_position);

        switch ((mode->bpp + 7) >> 3) {
            case 1:
                *(uint8_t *)dest = color & 0xFF;
                break;
            case 2:
                *(uint16_t *)dest = color & 0xFFFF;
                break;
            case 3:
                if ((uint32_t)dest & 1) {
                    *(uint8_t *)dest = color & 0xFF;
                    *(uint16_t *)((uint8_t *)dest + 1) = (color & 0xFFFF00) >> 8;
                } else if (count > 1) {
                    *(uint32_t *)dest = color;
                } else {
                    *(uint16_t *)dest = color & 0xFFFF;
                    *((uint8_t *)dest + 2) = (color & 0xFF0000) >> 16;
                }
                break;
            case 4:
                *(uint32_t *)dest = color;
                break;
        }

        dest = (uint8_t *)dest + ((mode->bpp + 7) >> 3);
        orig++;
        count--;
    }
}

static status_t setup_bitmap_buffer(struct device *dev, int width, int height, int bpp)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;
    status_t status;
    size_t hw_frame_size;

    if (data->char_buffer) {
        free(data->char_buffer);
        data->char_buffer = NULL;
    }

    data->frame_buffer = realloc(data->frame_buffer, width * height * sizeof(*data->frame_buffer));
    if (!data->frame_buffer) {
        status = STATUS_UNKNOWN_ERROR;
        goto has_error;
    }
    memset(data->frame_buffer, 0, width * height * sizeof(*data->frame_buffer));
    
    data->diff_buffer = realloc(data->diff_buffer, (width / DIFF_REGION_SIZE) * (height / DIFF_REGION_SIZE));
    if (!data->diff_buffer) {
        status = STATUS_UNKNOWN_ERROR;
        goto has_error;
    }
    memset(data->diff_buffer, 0, (width / DIFF_REGION_SIZE) * (height / DIFF_REGION_SIZE));

    hw_frame_size = data->mode_info.pitch * data->mode_info.height * data->mode_info.bpp / 8;
    mm_map(data->mode_info.framebuffer, (void *)data->mode_info.framebuffer, ALIGN((data->mode_info.lin_num_image_pages + 1) * hw_frame_size, 4096) >> 12, PF_WTCACHE);
    memset((void *)data->mode_info.framebuffer, 0, (data->mode_info.lin_num_image_pages + 1) * hw_frame_size);

    return STATUS_SUCCESS;

has_error:
    panic(status, "I don't know what more can I do (vbe, set_mode())");

    return status;
}

static status_t set_mode(struct device *dev, int width, int height, int bpp)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;
    status_t status;
    struct vbe_controller_info vbe_info;
    uint16_t *mode_list;
    struct vbe_video_mode_info vbe_mode_info;
    uint16_t mode = 0xFFFF;

    status = _pc_bios_vbe_get_controller_info(&vbe_info);
    if (!CHECK_SUCCESS(status)) goto has_error;

    mode_list = (uint16_t *)FARPTR_TO_VPTR(vbe_info.video_modes);
    for (int i = 0; mode_list[i] != 0xFFFF; i++) {
        status = _pc_bios_vbe_get_video_mode_info(mode_list[i], &vbe_mode_info);
        if (!CHECK_SUCCESS(status)) goto has_error;

        if (vbe_mode_info.width == width &&
            vbe_mode_info.height == height &&
            vbe_mode_info.bpp == bpp &&
            vbe_mode_info.memory_model == VBEMM_DIRECT) {
            mode = mode_list[i];
            memcpy(&data->mode_info, &vbe_mode_info, sizeof(vbe_mode_info));
            break;
        }
    }
    if (mode == 0xFFFF) {
        status = STATUS_ENTRY_NOT_FOUND;
        goto has_error;
    }

    status = _pc_bios_vbe_set_video_mode(mode);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = setup_bitmap_buffer(dev, width, height, bpp);
    if (!CHECK_SUCCESS(status)) goto has_error;

    if (vbe_mode_info.bpp == 24) {
        data->convert_color = convert_color_rgb888;
    } else if (vbe_mode_info.bpp == 32) {
        data->convert_color = convert_color_rgbx8888;
    } else if (vbe_mode_info.bpp == 16) {
        data->convert_color = convert_color_rgb565;
    } else {
        data->convert_color = convert_color_generic;
    }

    return STATUS_SUCCESS;

has_error:
    return status;
}

static status_t get_mode(struct device *dev, int *width, int *height, int *bpp)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    if (data->mode_info.memory_model != VBEMM_DIRECT) {
        return STATUS_CONFLICTING_STATE;
    }

    if (width) *width = data->mode_info.width;
    if (height) *height = data->mode_info.height;
    if (bpp) *bpp = data->mode_info.bpp;

    return STATUS_SUCCESS;
}

static status_t get_hw_mode(struct device *dev, struct fb_hw_mode *hwmode)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    hwmode->framebuffer = (void *)data->mode_info.framebuffer;
    hwmode->width = data->mode_info.width;
    hwmode->height = data->mode_info.height;
    hwmode->pitch = data->mode_info.pitch;
    hwmode->bpp = data->mode_info.bpp;
    switch (data->mode_info.memory_model) {
        case VBEMM_DIRECT:
            hwmode->memory_model = FBHWMMM_DIRECT;
            break;
        case VBEMM_TEXT:
            hwmode->memory_model = FBHWMMM_TEXT;
            break;
        default:
            return 1;
    }
    hwmode->rmask = data->mode_info.red_mask;
    hwmode->rpos = data->mode_info.red_position;
    hwmode->gmask = data->mode_info.green_mask;
    hwmode->gpos = data->mode_info.green_position;
    hwmode->bmask = data->mode_info.blue_mask;
    hwmode->bpos = data->mode_info.blue_position;

    return STATUS_SUCCESS;
}

static status_t get_framebuffer(struct device *dev, void **framebuffer)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;
    
    if (framebuffer) *framebuffer = data->frame_buffer;

    return STATUS_SUCCESS;
}

static status_t fb_invalidate(struct device *dev, int x0, int y0, int x1, int y1)
{
    for (int yr = y0 / DIFF_REGION_SIZE; yr < (y1 + DIFF_REGION_SIZE - 1) / DIFF_REGION_SIZE; yr++) {
        for (int xr = x0 / DIFF_REGION_SIZE; xr < (x1 + DIFF_REGION_SIZE - 1) / DIFF_REGION_SIZE; xr++) {
            set_region_diff(dev, xr, yr);
        }
    }

    return STATUS_SUCCESS;
}

static int get_diff_chunk(struct device *dev, int xr, int yr)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    int modified_chunk = get_region_diff(dev, xr, yr);
    int chunk_size = 1;

    while (++xr < data->mode_info.width / DIFF_REGION_SIZE) {
        if (modified_chunk != get_region_diff(dev, xr, yr)) break;
        chunk_size++;
    }

    return modified_chunk ? chunk_size : -chunk_size;
}

static status_t fb_flush(struct device *dev)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    for (int yr = 0; yr < data->mode_info.height / DIFF_REGION_SIZE; yr++) {
        int xr = 0;
        while (xr < data->mode_info.width / DIFF_REGION_SIZE) {
            int chunk_size = get_diff_chunk(dev, xr, yr);
            if (chunk_size < 0) {
                xr += -chunk_size;
                continue;
            }

            for (int i = 0; i < chunk_size; i++) {
                reset_region_diff(dev, xr + i, yr);
            }

            for (int y = yr * DIFF_REGION_SIZE; y < (yr + 1) * DIFF_REGION_SIZE; y++) {
                long fb_offs =
                    (y) * data->mode_info.pitch +
                    xr * DIFF_REGION_SIZE * data->mode_info.bpp / 8;

                data->convert_color(
                    &data->mode_info,
                    &data->frame_buffer[y * data->mode_info.width + xr * DIFF_REGION_SIZE],
                    (void *)(data->mode_info.framebuffer + fb_offs),
                    chunk_size * DIFF_REGION_SIZE
                );
            }

            xr += chunk_size;
        }
    }

    return STATUS_SUCCESS;
}

static status_t fb_present(struct device *dev)
{
    return STATUS_SUCCESS;
}

static const struct framebuffer_interface fbif = {
    .set_mode = set_mode,
    .get_mode = get_mode,
    .get_hw_mode = get_hw_mode,
    .get_framebuffer = get_framebuffer,
    .invalidate = fb_invalidate,
    .flush = fb_flush,
    .present = fb_present,
};

static status_t setup_text_buffer(struct device *dev, int width, int height)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;
    status_t status;

    if (data->frame_buffer) {
        free(data->frame_buffer);
        data->frame_buffer = NULL;
    }

    data->char_buffer = realloc(data->char_buffer, width * height * sizeof(*data->char_buffer));
    if (!data->char_buffer) {
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

    return STATUS_SUCCESS;

has_error:
    panic(status, "I don't know what more can I do");

    return status;
}

static status_t set_dimension(struct device *dev, int width, int height)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;
    status_t status;
    struct vbe_controller_info vbe_info;
    uint16_t *mode_list;
    struct vbe_video_mode_info vbe_mode_info;
    uint16_t mode = 0xFFFF;

    status = _pc_bios_vbe_get_controller_info(&vbe_info);
    if (!CHECK_SUCCESS(status)) goto has_error;

    mode_list = (uint16_t *)FARPTR_TO_VPTR(vbe_info.video_modes);
    for (int i = 0; mode_list[i] != 0xFFFF; i++) {
        status = _pc_bios_vbe_get_video_mode_info(mode_list[i], &vbe_mode_info);
        if (!CHECK_SUCCESS(status)) goto has_error;

        if (vbe_mode_info.width == width &&
            vbe_mode_info.height == height &&
            vbe_mode_info.memory_model == VBEMM_TEXT) {
            mode = mode_list[i];
            memcpy(&data->mode_info, &vbe_mode_info, sizeof(vbe_mode_info));
            break;
        }
    }
    if (mode == 0xFFFF) {
        status = STATUS_ENTRY_NOT_FOUND;
        goto has_error;
    }

    status = _pc_bios_vbe_set_video_mode(mode);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = setup_text_buffer(dev, width, height);
    if (!CHECK_SUCCESS(status)) goto has_error;

    if (!vbe_mode_info.framebuffer) {
        memset((void *)0xB8000, 0, width * height * sizeof(uint16_t));
    }

    _pc_bios_video_set_cursor_pos(0, 0, 0);

    return STATUS_SUCCESS;

has_error:
    return status;
}

static status_t get_dimension(struct device *dev, int *width, int *height)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    if (data->mode_info.memory_model != VBEMM_TEXT) {
        return STATUS_CONFLICTING_STATE;
    }

    if (width) *width = data->mode_info.width;
    if (height) *height = data->mode_info.height;

    return STATUS_SUCCESS;
}

static status_t get_buffer(struct device *dev, struct console_char_cell **buf)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    if (buf) *buf = data->char_buffer;
    
    return STATUS_SUCCESS;
}

static status_t con_invalidate(struct device *dev, int x0, int y0, int x1, int y1)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            data->diff_buffer[(y * data->mode_info.width + x) / 8] |= 1 << ((y * data->mode_info.width + x) % 8);
        }
    }

    return STATUS_SUCCESS;
}

static status_t con_flush(struct device *dev) {
    return STATUS_SUCCESS;
}

static status_t con_present(struct device *dev)
{
    struct vbe_data *data = (struct vbe_data*)dev->data;
    status_t status;
    const struct console_char_cell *src;
    uint8_t cp437_char;
    uint16_t cell;

    for (int y = 0; y < data->mode_info.height; y++) {
        for (int x = 0; x < data->mode_info.width; x++) {
            if (!(data->diff_buffer[(y * data->mode_info.width + x) / 8] &
                (1 << ((y * data->mode_info.width + x) % 8)))) {
                continue;
            }

            src = &data->char_buffer[y * data->mode_info.width + x];
            status = enc_utf32_to_cp437(src->codepoint, &cp437_char);
            if (!CHECK_SUCCESS(status)) {
                cp437_char = 0;
            }
            cell = cp437_char;
            cell |= (rgb_to_irgb(src->attr.text_reversed ? src->attr.bg_color : src->attr.fg_color) & 0xF) << 8;
            cell |= (rgb_to_irgb(src->attr.text_reversed ? src->attr.fg_color : src->attr.bg_color) & 0xF) << 12;
            if (src->attr.text_dim) {
                cell &= 0x77FF;
            }
            ((uint16_t *)0xB8000)[y * data->mode_info.width + x] = cell;
            data->diff_buffer[(y * data->mode_info.width + x) / 8] &= ~(1 << ((y * data->mode_info.width + x) % 8));
        }
    }

    return STATUS_SUCCESS;
}

static status_t set_cursor_pos(struct device *dev, int col, int row)
{
    if (col < 0 || row < 0) {
        uint8_t row_prev, col_prev;
        _pc_bios_video_get_cursor_info(0, NULL, row < 0 ? &row_prev : NULL, col < 0 ? &col_prev : NULL);
        if (col < 0) {
            col = col_prev;
        }
        if (row < 0) {
            row = row_prev;
        }
    }

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

static void vbe_init(void)
{
    status_t status;
    struct device_driver *drv;

    status = device_driver_create(&drv);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "cannot register device driver \"vbe\"");
    }

    drv->name = "vbe";
    drv->probe = probe;
    drv->remove = remove;
    drv->get_interface = get_interface;
}

static status_t probe(struct device **devout, struct device_driver *drv, struct device *parent, struct resource *rsrc, int rsrc_cnt)
{
    status_t status;
    struct vbe_controller_info vbe_info;
    struct device *dev = NULL;
    farptr_t pmi_table;
    uint16_t pmi_table_size;
    struct vbe_data *data = NULL;
    uint16_t vmode;
    struct vbe_video_mode_info vmode_info;

    status = _pc_bios_vbe_get_controller_info(&vbe_info);
    if (!CHECK_SUCCESS(status)) goto has_error;
    
    status = device_create(&dev, drv, parent);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = device_generate_name("video", dev->name, sizeof(dev->name));
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = _pc_bios_vbe_get_pm_interface(&pmi_table, &pmi_table_size);
    if (!CHECK_SUCCESS(status)) {
        pmi_table.segment = 0;
        pmi_table.offset = 0;
    }

    data = malloc(sizeof(*data));
    data->char_buffer = NULL;
    data->frame_buffer = NULL;
    data->diff_buffer = NULL;
    data->current_page = 0;
    data->pmi_table = pmi_table;
    dev->data = data;

    status = _pc_bios_vbe_get_video_mode(&vmode);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = _pc_bios_vbe_get_video_mode_info(vmode, &vmode_info);
    if (!CHECK_SUCCESS(status)) goto has_error;

    switch (vmode_info.memory_model) {
        case VBEMM_DIRECT:
            status = setup_bitmap_buffer(dev, vmode_info.width, vmode_info.height, vmode_info.bpp);
            if (!CHECK_SUCCESS(status)) goto has_error;
            break;
        case VBEMM_TEXT:
            status = setup_text_buffer(dev, vmode_info.width, vmode_info.height);
            if (!CHECK_SUCCESS(status)) goto has_error;
            break;
    }
    
    if (devout) *devout = dev;

    return STATUS_SUCCESS;

has_error:
    if (data) {
        free(data);
    }

    if (dev) {
        device_remove(dev);
    }

    for (;;) {}

    return status;
}

static status_t remove(struct device *dev)
{
    struct vbe_data *data = (struct vbe_data *)dev->data;

    if (data->char_buffer) {
        free(data->char_buffer);
    }

    if (data->frame_buffer) {
        _pc_bios_vbe_set_display_start(0, 0);
        free(data->frame_buffer);
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
    if (strcmp(name, "framebuffer") == 0) {
        if (result) *result = &fbif;
        return STATUS_SUCCESS;
    } else if (strcmp(name, "console") == 0) {
        if (result) *result = &conif;
        return STATUS_SUCCESS;
    }

    return STATUS_ENTRY_NOT_FOUND;
}

DEVICE_DRIVER(vbe, vbe_init)

