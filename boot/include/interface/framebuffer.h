#ifndef __INTERFACE_FRAMEBUFFER_H__
#define __INTERFACE_FRAMEBUFFER_H__

#include <device/device.h>

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
    int (*set_mode)(struct device *, int, int, int);
    int (*get_mode)(struct device *, int *, int *, int *);
    int (*get_hw_mode)(struct device *, struct fb_hw_mode *);
    void *(*get_framebuffer)(struct device *);
    void (*invalidate)(struct device *, int, int, int, int);
    void (*flush)(struct device *);
    void (*present)(struct device *);
};

#endif // __INTERFACE_FRAMEBUFFER_H__
