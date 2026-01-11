#ifndef __EMOS_DEVICE_H__
#define __EMOS_DEVICE_H__

#include <emos/types.h>
#include <emos/status.h>
#include <emos/uuid.h>
#include <emos/object.h>

#define DEVICE(obj) (CHECK_OBJECT_TYPE(obj, OT_DEVICE) ? ((struct device *)obj) : NULL)

struct bus;
struct device_driver;

struct device {
    struct object obj;
    struct device_driver *driver;
    void *data;
};

struct device_driver_ops {
    status_t (*deinit)(struct device_driver *drv);
    status_t (*early_deinit)(struct device_driver *drv);

    status_t (*probe)(struct device **devout, struct device_driver *drv, struct bus *bus, void *args);
    status_t (*remove)(struct device *dev);
};

struct device_driver {
    struct device_driver_ops *ops;
};

status_t device_create(struct device **devout, struct bus *bus);
void device_remove(struct device *dev);

status_t device_driver_create(struct device_driver **drv);
void device_driver_remove(struct device_driver *drv);

status_t device_driver_add_interface(struct device_driver *drv, struct uuid if_uuid, const void *interface);
status_t emos_device_driver_get_interface(struct device_driver *drv, struct uuid if_uuid, const void **ifout);

#endif // __EMOS_DEVICE_DRIVER_H__
