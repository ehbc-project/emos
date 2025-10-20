#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <mm/mm.h>
#include <device/device.h>
#include <device/driver.h>

struct rtc_data {
    int dummy;
};

static int probe(struct device *dev);
static int remove(struct device *dev);
static const void *get_interface(struct device *dev, const char *name);

static struct device_driver drv = {
    .name = "rtc_isa",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static int probe(struct device *dev)
{
    dev->name = "rtc";
    dev->id = generate_device_id(dev->name);

    struct rtc_data *data = mm_allocate(sizeof(*data));

    dev->data = data;

    return 0;
}

static int remove(struct device *dev)
{
    struct rtc_data *data = (struct rtc_data *)dev->data;

    mm_free(data);

    return 0;
}

static const void *get_interface(struct device *dev, const char *name)
{
    return NULL;
}

DEVICE_DRIVER(drv)

