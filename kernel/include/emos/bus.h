#ifndef __EMOS_BUS_H__
#define __EMOS_BUS_H__

#include <emos/types.h>
#include <emos/status.h>
#include <emos/device.h>
#include <emos/uuid.h>
#include <emos/object.h>

#define BUS(obj) (CHECK_OBJECT_TYPE(obj, OT_BUS) ? ((struct bus *)obj) : NULL)

struct bus_driver;

struct bus {
    struct object obj;
    struct bus_driver *driver;
    void *data;
};

struct bus_driver_ops {
    status_t (*deinit)(struct bus_driver *drv);
    status_t (*early_deinit)(struct bus_driver *drv);

    status_t (*scan)(struct bus *bus);
};

struct bus_driver {
    struct uuid id;
    struct bus_driver_ops *ops;
};

status_t emos_bus_create(struct bus **busout);
status_t emos_bus_remove(struct bus *bus);
status_t emos_bus_driver_create(struct bus_driver **drv);
status_t emos_bus_driver_remove(struct bus_driver *drv);
status_t emos_bus_driver_add_interface(struct bus_driver *drv, struct uuid if_uuid, const void *interface);
status_t emos_bus_driver_get_interface(struct bus_driver *drv, struct uuid if_uuid, const void **ifout);

#endif // __EMOS_BUS_H__
