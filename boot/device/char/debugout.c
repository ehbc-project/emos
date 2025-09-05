#include <string.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <interface/char.h>

#include <asm/io.h>

static long write(struct device *dev, const char *buf, unsigned long len)
{
    for (int i = 0; i < len; i++) {
        switch (dev->resource->type) {
            case RT_IOPORT:
                _i686_out8(dev->resource->base, buf[i]);
                break;
            case RT_MEMORY:
                *(uint8_t *)(long)dev->resource->base = buf[i];
                break;
            default:
                break;
        }
    }

    return len;
}

static const struct char_interface charif = {
    .write = write,
};

static int probe(struct device *dev);
static int remove(struct device *dev);
static const void *get_interface(struct device *dev, const char *name);

static struct device_driver drv = {
    .name = "debugout",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static int probe(struct device *dev)
{
    if (!dev->resource) return 1;
    if (dev->resource->type != RT_MEMORY && dev->resource->type != RT_IOPORT) {
        return 1;
    }

    dev->name = "dbg";
    dev->id = generate_device_id(dev->name);

    return 0;
}

static int remove(struct device *dev)
{
    return 0;
}

static const void *get_interface(struct device *dev, const char *name)
{
    if (strcmp(name, "char") == 0) {
        return &charif;
    }

    return NULL;
}

__attribute__((constructor))
static void _register_driver(void)
{
    register_device_driver(&drv);
}
