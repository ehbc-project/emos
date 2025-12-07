#include <string.h>
#include <stdlib.h>

#include <eboot/status.h>
#include <eboot/device.h>
#include <eboot/interface/block.h>
#include <eboot/interface/ide.h>

#include "ata.h"
#include "atapi.h"

struct atapi_data {
    int slave;
};

static status_t read(struct device *dev, lba_t lba, void *buf, size_t count, size_t *result)
{
    return STATUS_UNIMPLEMENTED;
}

static status_t write(struct device *dev, lba_t lba, const void *buf, size_t count, size_t *result)
{
    return STATUS_UNIMPLEMENTED;
}

static const struct block_interface blkif = {
    .read = read,
    .write = write,
};

static status_t probe(struct device **devout, struct device_driver *drv, struct device *parent, struct resource *rsrc, int rsrc_cnt);
static status_t remove(struct device *dev);
static status_t get_interface(struct device *dev, const char *name, const void **result);
    
static void atapi_init(void)
{
    status_t status;
    struct device_driver *drv;

    status = device_driver_create(&drv);
    if (!CHECK_SUCCESS(status)) {
        panic("cannot register device driver \"atapi\"");
    }

    drv->name = "atapi";
    drv->probe = probe;
    drv->remove = remove;
    drv->get_interface = get_interface;
}

static status_t probe(struct device **devout, struct device_driver *drv, struct device *parent, struct resource *rsrc, int rsrc_cnt)
{
    status_t status;
    struct device *dev = NULL;
    struct device *idedev = NULL;
    const struct ide_interface *ideif = NULL;
    struct atapi_data *data = NULL;

    if (!rsrc || rsrc_cnt != 1 || rsrc[0].type != RT_BUS || rsrc[0].base != rsrc[0].limit || rsrc[0].base > 1) {
        status = STATUS_INVALID_RESOURCE;
        goto has_error;
    }

    idedev = parent;
    if (!idedev) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }
    
    status = idedev->driver->get_interface(idedev, "ide", (const void **)&ideif);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = device_create(&dev, drv, parent);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = device_generate_name("rd", dev->name, sizeof(dev->name));
    if (!CHECK_SUCCESS(status)) goto has_error;

    data = malloc(sizeof(*data));
    data->slave = 0;
    dev->data = data;
    
    if (devout) *devout = dev;

    return STATUS_SUCCESS;

has_error:
    if (data) {
        free(data);
    }

    if (dev) {
        device_remove(dev);
    }

    return status;
}

static status_t remove(struct device *dev)
{
    struct atapi_data *data = (struct atapi_data *)dev->data;

    free(data);

    device_remove(dev);

    return STATUS_SUCCESS;
}

static status_t get_interface(struct device *dev, const char *name, const void **result)
{
    if (strcmp(name, "block") == 0) {
        if (result) *result = &blkif;
        return STATUS_SUCCESS;
    }

    return STATUS_ENTRY_NOT_FOUND;
}

DEVICE_DRIVER(atapi, atapi_init)
