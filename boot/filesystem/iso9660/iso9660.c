#include <string.h>
#include <stdlib.h>

#include <eboot/compiler.h>
#include <eboot/device.h>
#include <eboot/filesystem.h>
#include <eboot/interface/block.h>

struct iso9660_data {
    int a;
};

static status_t probe(struct device *dev, struct fs_driver *drv);
static status_t mount(struct filesystem **fsout, struct fs_driver *drv, struct device *dev, const char *name);
static status_t unmount(struct filesystem *fs);

static void iso9660_init(void)
{
    status_t status;
    struct fs_driver *drv;

    status = filesystem_driver_create(&drv);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "cannot register fs driver \"afs\"");
    }

    drv->name = "afs";
    drv->probe = probe;
    drv->mount = mount;
    drv->unmount = unmount;
}

static status_t probe(struct device *dev, struct fs_driver *drv)
{
    return STATUS_UNIMPLEMENTED;
}

static status_t mount(struct filesystem **fsout, struct fs_driver *drv, struct device *dev, const char *name)
{
    return STATUS_UNIMPLEMENTED;
}

static status_t unmount(struct filesystem *fs)
{
    return STATUS_UNIMPLEMENTED;
}

FILESYSTEM_DRIVER(iso9660, iso9660_init)
