#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <eboot/device.h>

struct rtc_data {
    int dummy;
};

static status_t probe(struct device **devout, struct device_driver *drv, struct device *parent, struct resource *rsrc, int rsrc_cnt);
static status_t remove(struct device *dev);
static status_t get_interface(struct device *dev, const char *name, const void **result);
    
static void rtc_isa_init(void)
{
    status_t status;
    struct device_driver *drv;

    status = device_driver_create(&drv);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "cannot register device driver \"rtc_isa\"");
    }

    drv->name = "rtc_isa";
    drv->probe = probe;
    drv->remove = remove;
    drv->get_interface = get_interface;
}

static status_t probe(struct device **devout, struct device_driver *drv, struct device *parent, struct resource *rsrc, int rsrc_cnt)
{
    status_t status;
    struct device *dev = NULL;
    struct rtc_data *data = NULL;

    status = device_create(&dev, drv, parent);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = device_generate_name("rtc", dev->name, sizeof(dev->name));
    if (!CHECK_SUCCESS(status)) goto has_error;

    data = malloc(sizeof(*data));
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
    struct rtc_data *data = (struct rtc_data *)dev->data;

    free(data);

    device_remove(dev);

    return STATUS_SUCCESS;
}

static status_t get_interface(struct device *dev, const char *name, const void **result)
{
    return STATUS_ENTRY_NOT_FOUND;
}

DEVICE_DRIVER(rtc_isa, rtc_isa_init)

