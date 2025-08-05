#ifndef __INTERFACE_FRAMEBUFFER_H__
#define __INTERFACE_FRAMEBUFFER_H__

#include <device/device.h>

struct framebuffer_interface {
    int (*set_mode)(struct device *, int, int, int);
    int (*get_mode)(struct device *, int *, int *, int *);
    void *(*get_framebuffer)(struct device *);
    void (*invalidate)(struct device *, int, int, int, int);
    void (*flush)(struct device *);
    void (*present)(struct device *);
};

#endif // __INTERFACE_FRAMEBUFFER_H__
