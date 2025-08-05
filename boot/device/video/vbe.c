#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <mm/mm.h>
#include <device/device.h>
#include <device/driver.h>
#include <device/video/vbe.h>
#include <interface/framebuffer.h>
#include <asm/bios/video.h>
#include <core/panic.h>

#define DIFF_REGION_SIZE 16

static uint32_t convert_color(struct vbe_video_mode_info *vbe_mode_info, uint32_t color)
{
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    r >>= 8 - vbe_mode_info->red_mask;
    g >>= 8 - vbe_mode_info->green_mask;
    b >>= 8 - vbe_mode_info->blue_mask;

    return 
        (r << vbe_mode_info->red_position) |
        (g << vbe_mode_info->green_position) |
        (b << vbe_mode_info->blue_position);
}

inline static uint32_t get_pixel(struct vbe_device *dev, int x, int y)
{
    return dev->frame_buffer[y * dev->mode_info.width + x];
}

inline static uint32_t get_fb_offs(struct vbe_video_mode_info *vbe_mode_info, int page, int x, int y)
{
    return
        page * vbe_mode_info->height * vbe_mode_info->pitch +
        y * vbe_mode_info->pitch +
        x * vbe_mode_info->bpp / 8;
}

inline static void set_region_diff(struct vbe_device *dev, int xregion, int yregion)
{
    dev->diff_buffer[yregion * dev->mode_info.width + xregion] = 3;
}

inline static void reset_region_diff(struct vbe_device *dev, int page, int xregion, int yregion)
{
    dev->diff_buffer[yregion * dev->mode_info.width + xregion] &= ~(1 << page);
}

inline static int get_region_diff(struct vbe_device *dev, int page, int xregion, int yregion)
{
    return (dev->diff_buffer[yregion * dev->mode_info.width + xregion] >> page) & 1;
}

static int set_mode(struct device *_dev, int width, int height, int bpp)
{
    struct vbe_device *dev = (struct vbe_device *)_dev;

    struct vbe_controller_info vbe_info;
    _pc_bios_get_vbe_controller_info(&vbe_info);

    uint16_t *mode_list = (uint16_t*)((vbe_info.video_modes.segment << 4) + vbe_info.video_modes.offset);
    struct vbe_video_mode_info vbe_mode_info;
    uint16_t mode = 0xFFFF;
    for (int i = 0; mode_list[i] != 0xFFFF; i++) {
        _pc_bios_get_vbe_video_mode_info(mode_list[i], &vbe_mode_info);

        if (vbe_mode_info.width == width && vbe_mode_info.height == height && vbe_mode_info.bpp == bpp) {
            mode = mode_list[i];
            memcpy(&dev->mode_info, &vbe_mode_info, sizeof(vbe_mode_info));
            break;
        }
    }
    if (mode == 0xFFFF) {
        return ENOENT;
    }

    int err = _pc_bios_set_vbe_video_mode(mode);
    if (err) {
        return err;
    }

    dev->frame_buffer = mm_reallocate(dev->frame_buffer, width * height * sizeof(uint32_t));
    memset(dev->frame_buffer, 0, width * height * sizeof(uint32_t));
    
    dev->diff_buffer = mm_reallocate(dev->diff_buffer, (width / DIFF_REGION_SIZE) * (height / DIFF_REGION_SIZE));
    memset(dev->diff_buffer, 0, (width / DIFF_REGION_SIZE) * (height / DIFF_REGION_SIZE));

    memset((void *)dev->mode_info.framebuffer, 0, 2 * dev->mode_info.pitch * dev->mode_info.height * dev->mode_info.bpp / 8);

    return 0;
}

static int get_mode(struct device *_dev, int *width, int *height, int *bpp)
{
    struct vbe_device *dev = (struct vbe_device *)_dev;

    if (width) *width = dev->mode_info.width;
    if (height) *height = dev->mode_info.height;
    if (bpp) *bpp = dev->mode_info.bpp;

    return 0;
}

static void *get_framebuffer(struct device *_dev)
{
    struct vbe_device *dev = (struct vbe_device *)_dev;
    
    return dev->frame_buffer;
}

static void invalidate(struct device *_dev, int x0, int y0, int x1, int y1)
{
    struct vbe_device *dev = (struct vbe_device *)_dev;
    
    for (int yr = y0 / DIFF_REGION_SIZE; yr <= (y1 + DIFF_REGION_SIZE - 1) / DIFF_REGION_SIZE; yr++) {
        for (int xr = x0 / DIFF_REGION_SIZE; xr <= (x1 + DIFF_REGION_SIZE - 1) / DIFF_REGION_SIZE; xr++) {
            set_region_diff(dev, xr, yr);
        }
    }
}

static void flush(struct device *_dev)
{
    struct vbe_device *dev = (struct vbe_device *)_dev;

    int next_page = (dev->current_page + 1) % 2;

    for (int yr = 0; yr < dev->mode_info.height / DIFF_REGION_SIZE; yr++) {
        for (int xr = 0; xr < dev->mode_info.width / DIFF_REGION_SIZE; xr++) {
            if (!get_region_diff(dev, next_page, xr, yr)) {
                continue;
            }
            reset_region_diff(dev, next_page, xr, yr);

            for (int y = yr * DIFF_REGION_SIZE; y < (yr + 1) * DIFF_REGION_SIZE; y++) {
                for (int x = xr * DIFF_REGION_SIZE; x < (xr + 1) * DIFF_REGION_SIZE; x++) {
                    void *pixel = (void *)(dev->mode_info.framebuffer + get_fb_offs(&dev->mode_info, next_page, x, y));

                    uint32_t color = convert_color(&dev->mode_info, get_pixel(dev, x, y));
                    
                    switch (dev->mode_info.bpp) {
                        case 1:
                            *(uint8_t*)pixel &= ~(1 << (x & 3));
                            *(uint8_t*)pixel |= color << (x & 3);
                        case 4:
                            *(uint8_t*)pixel &= (x & 1) ? 0x0F : 0xF0;
                            *(uint8_t*)pixel |= color;
                            break;
                        case 8:
                            *(uint8_t*)pixel = color;
                            break;
                        case 16:
                            *(uint16_t*)pixel = color;
                            break;
                        case 24:
                            *(uint16_t*)pixel = color & 0xFFFF;
                            ((uint8_t*)pixel)[2] = (color >> 16) & 0xFF;
                            break;
                        case 32:
                            *(uint32_t*)pixel = color;
                            break;
                    }
                }
            }
        }
    }
}

static void present(struct device *_dev)
{
    struct vbe_device *dev = (struct vbe_device *)_dev;

    dev->current_page = (dev->current_page + 1) % 2;
    _pc_bios_set_vbe_display_start(0, dev->current_page * dev->mode_info.height);
}

static const struct framebuffer_interface fbif = {
    .set_mode = set_mode,
    .get_mode = get_mode,
    .get_framebuffer = get_framebuffer,
    .invalidate = invalidate,
    .flush = flush,
    .present = present,
};

static struct device *probe(const struct device_id *id);
static int remove(struct device *_dev);
static const void *get_interface(struct device *_dev, const char *name);

static struct device_driver drv = {
    .name = "vbe_video",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static struct device *probe(const struct device_id *id)
{
    struct vbe_controller_info vbe_info;
    if (_pc_bios_get_vbe_controller_info(&vbe_info)) return NULL;
    
    struct vbe_device *dev = mm_allocate(sizeof(*dev));
    if (!dev) return NULL;
    dev->dev.driver = &drv;

    struct device_id *current_id = mm_allocate(sizeof(*current_id));
    current_id->type = DIT_STRING;
    current_id->string = "video";
    dev->dev.id = current_id;

    dev->fbif = &fbif;

    dev->frame_buffer = NULL;
    dev->diff_buffer = NULL;
    dev->current_page = 0;

    register_device((struct device *)dev);

    return (struct device *)dev;
}

static int remove(struct device *_dev)
{
    struct vbe_device *dev = (struct vbe_device *)_dev;

    if (dev->frame_buffer) mm_free(dev->frame_buffer);
    if (dev->diff_buffer) mm_free(dev->diff_buffer);

    mm_free(dev);

    return 0;
}

static const void *get_interface(struct device *_dev, const char *name)
{
    struct vbe_device *dev = (struct vbe_device *)_dev;

    if (strcmp(name, "framebuffer") == 0) {
        return dev->fbif;
    }

    return NULL;
}

__attribute__((constructor))
static void _register_driver(void)
{
    register_driver((struct device_driver *)&drv);
}

