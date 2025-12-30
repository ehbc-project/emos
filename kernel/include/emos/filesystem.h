#ifndef __EMOS_FILESYSTEM_H__
#define __EMOS_FILESYSTEM_H__

#include <emos/types.h>
#include <emos/status.h>
#include <emos/device.h>
#include <emos/uuid.h>
#include <emos/object.h>

#define FILESYSTEM(obj) (CHECK_OBJECT_TYPE(obj, OT_FILESYSTEM) ? ((struct filesystem *)obj) : NULL)

struct filesystem_driver;

struct filesystem {
    struct object obj;
    struct filesystem_driver *driver;
    struct device *dev;
    void *data;
};

struct filesystem_driver_ops {
    status_t (*deinit)(struct filesystem_driver *drv);
    status_t (*early_deinit)(struct filesystem_driver *drv);

    status_t (*probe)(struct device *dev);
    status_t (*mount)(struct filesystem **fsout, struct device *dev);
    status_t (*unmount)(struct filesystem *fs);
};

struct filesystem_driver {
    struct filesystem_driver_ops *ops;
};

status_t emos_filesystem_create(struct filesystem **fsout);
status_t emos_filesystem_remove(struct filesystem *fs);

status_t emos_filesystem_driver_create(struct filesystem_driver **drv);
status_t emos_filesystem_driver_remove(struct filesystem_driver *drv);
status_t emos_filesystem_driver_add_interface(struct filesystem_driver *drv, struct uuid if_uuid, const void *interface);
status_t emos_filesystem_driver_get_interface(struct filesystem_driver *drv, struct uuid if_uuid, const void **ifout);

#endif // __EMOS_FILESYSTEM_H__
