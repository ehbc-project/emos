#ifndef __EBOOT_INTERFACE_FRAMEBUFFER_H__
#define __EBOOT_INTERFACE_FRAMEBUFFER_H__

#include <eboot/status.h>
#include <eboot/device.h>

struct framebuffer_interface {
    status_t (*get_framebuffer)(struct device *, void **);
    status_t (*invalidate)(struct device *, int, int, int, int);
    status_t (*flush)(struct device *);
};

#endif // __EBOOT_INTERFACE_FRAMEBUFFER_H__
