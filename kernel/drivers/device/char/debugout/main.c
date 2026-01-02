#include <stdint.h>
#include <stdalign.h>

#include <emos/asm/ioport.h>
#include <emos/types.h>
#include <emos/status.h>
#include <emos/log.h>
#include <emos/uuid.h>
#include <emos/memory.h>
#include <emos/object.h>
#include <emos/device.h>
#include <emos/interface.h>
#include <emos/interface/char.h>
#include <emos/bus.h>
#include <emos/bus/nonpnp.h>

struct device_data {
    struct nonpnp_resource res;
};

static const struct char_interface charif;

static status_t probe(struct device **devout, struct device_driver *drv, struct bus *bus, void *probe_args);
static status_t remove(struct device *dev);

int main(int argc, char **argv)
{
    status_t status;
    struct device_driver *drv = NULL;

    emos_log(LOG_INFO, "init device/char/debugout\n");

    status = device_driver_create(&drv);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "device_driver_create() failed: 0x%08X\n", status);
        goto has_error;
    }

    drv->ops->early_deinit = early_deinit;
    drv->ops->deinit = deinit;
    drv->ops->probe = probe;
    drv->ops->remove = remove;

    status = device_driver_add_interface(drv, CHAR_INTERFACE_UUID, &charif);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "device_driver_add_interface() failed: 0x%08X\n", status);
        goto has_error;
    }

    return STATUS_SUCCESS;

has_error:
    if (drv) {
        device_driver_remove(drv);
    }

    return status;
}

static status_t early_deinit(struct device_driver *drv)
{
    return STATUS_SUCCESS;
}

static status_t deinit(struct device_driver *drv)
{
    device_driver_remove(drv);

    return STATUS_SUCCESS;
}

static status_t probe(struct device **devout, struct device_driver *drv, struct bus *bus, void *probe_args)
{
    status_t status;
    struct device *dev = NULL;
    struct device_data *data = NULL;
    struct nonpnp_probe_args *args;

    if (!emos_uuid_isequal(bus->driver->id, NONPNP_BUS_UUID)) {
        emos_log(LOG_ERROR, "unsupported bus type\n");
        goto has_error;
    }

    args = probe_args;
    if (args->resource_count != 1) {
        emos_log(LOG_ERROR, "invalid resource count\n");
        status = STATUS_INVALID_RESOURCE;
        goto has_error;
    }
    if (args->resources[0].type != RT_IOPORT) {
        emos_log(LOG_ERROR, "invalid resource type\n");
        status = STATUS_INVALID_RESOURCE;
        goto has_error;
    }
    if (args->resources[0].start != args->resources[0].end) {
        emos_log(LOG_ERROR, "invalid resource range\n");
        status = STATUS_INVALID_RESOURCE;
        goto has_error;
    }

    status = device_create(&dev, bus);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "device_create() failed: 0x%08X\n", status);
        goto has_error;
    }

    status = emos_memory_allocate((void **)&data, sizeof(struct device_data), alignof(struct device_data));
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "emos_memory_allocate() failed: 0x%08X\n", status);
        goto has_error;
    }

    data->res = args->resources[0];
    dev->data = data;

    if (devout) *devout = dev;

    return STATUS_SUCCESS;

has_error:
    if (data) {
        emos_memory_free(data);
    }

    if (dev) {
        device_remove(dev);
    }

    return status;
}

static status_t remove(struct device *dev)
{
    status_t status;
    struct device_data *data = dev->data;

    emos_memory_free(data);
    device_remove(dev);

    return STATUS_SUCCESS;
}

static status_t charif_write(struct object *obj, const char *buf, size_t count, size_t *result)
{
    struct device *dev = DEVICE(obj);
    struct device_data *data = dev->data;

    for (size_t i = 0; i < count; i++) {
        emos_ioport_write8(data->res.start, buf[i]);
    }

    if (result) *result = count;

    return STATUS_SUCCESS;
}

static const struct char_interface charif = {
    .write = charif_write,
};
