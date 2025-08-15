#ifndef __DEVICE_DRIVER_H__
#define __DEVICE_DRIVER_H__

#include <device/device.h>

struct device_driver {
    struct device_driver *next;

    const char *name;
    int (*probe)(struct device *);
    int (*remove)(struct device *);
    const void *(*get_interface)(struct device *, const char *);
};

void register_driver(struct device_driver *drv);

struct device_driver *find_driver(const char *name);

#endif // __DEVICE_DRIVER_H__
