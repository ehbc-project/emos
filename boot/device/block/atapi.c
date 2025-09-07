#include <string.h>

#include <mm/mm.h>
#include <bus/bus.h>
#include <device/driver.h>
#include <interface/block.h>

struct atapi_data {
    int slave;
};

static long read(struct device *dev, lba_t lba, void *buf, long count)
{
    return -1;
}

static long write(struct device *dev, lba_t lba, const void *buf, long count)
{
    return -1;
}

static const struct block_interface blkif = {
    .read = read,
    .write = write,
};

static int probe(struct device *dev);
static int remove(struct device *dev);
static const void *get_interface(struct device *dev, const char *name);

static struct device_driver drv = {
    .name = "atapi",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static int probe(struct device *dev)
{
    dev->name = "rd";
    dev->id = generate_device_id(dev->name);

    struct atapi_data *data = mm_allocate(sizeof(*data));
    data->slave = 0;

    dev->data = data;

    return 0;
}

static int remove(struct device *dev)
{
    struct atapi_data *data = (struct atapi_data *)dev->data;

    mm_free(data);

    return 0;
}

static const void *get_interface(struct device *dev, const char *name)
{
    if (strcmp(name, "block") == 0) {
        return &blkif;
    }

    return NULL;
}

__attribute__((constructor))
static void _register_driver(void)
{
    register_device_driver(&drv);
}
