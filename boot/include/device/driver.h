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

/**
 * @brief Register a device driver.
 * @param drv A pointer to the device driver structure to register.
 */
void register_device_driver(struct device_driver *drv);

/**
 * @brief Find a device driver by name.
 * @param name The name of the driver to find.
 * @return A pointer to the found device driver, or NULL if not found.
 */
struct device_driver *find_device_driver(const char *name);

#endif // __DEVICE_DRIVER_H__
