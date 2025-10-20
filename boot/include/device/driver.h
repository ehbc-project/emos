#ifndef __DEVICE_DRIVER_H__
#define __DEVICE_DRIVER_H__

#include <device/device.h>

#include <compiler.h>

struct device_driver {
    struct device_driver *next;

    const char *name;
    int (*probe)(struct device *);
    int (*remove)(struct device *);
    const void *(*get_interface)(struct device *, const char *);
};

void register_device_driver(struct device_driver *drv);

struct device_driver *find_device_driver(const char *name);

#define DEVICE_DRIVER(drv) \
    __constructor \
    static void _register_driver_##drv(void) \
    { \
        register_device_driver(&drv); \
    }

#endif // __DEVICE_DRIVER_H__
