#ifndef __EBOOT_INTERFACE_FRAMEBUFFER_H__
#define __EBOOT_INTERFACE_FRAMEBUFFER_H__

#include <eboot/status.h>
#include <eboot/device.h>

#define FBHWMMM_DIRECT 0
#define FBHWMMM_TEXT   1

struct fb_hw_mode {
    void *framebuffer;
    int width;
    int height;
    int pitch;
    int bpp;
    int memory_model;
    int rmask;
    int rpos;
    int gmask;
    int gpos;
    int bmask;
    int bpos;
};

struct framebuffer_interface {
    status_t (*set_mode)(struct device *, int, int, int);
    status_t (*get_mode)(struct device *, int *, int *, int *);
    status_t (*get_hw_mode)(struct device *, struct fb_hw_mode *);
    status_t (*get_framebuffer)(struct device *, void **);
    status_t (*invalidate)(struct device *, int, int, int, int);
    status_t (*flush)(struct device *);
    status_t (*present)(struct device *);
};

#endif // __EBOOT_INTERFACE_FRAMEBUFFER_H__
