#include <string.h>
#include <stdio.h>

#include <mm/mm.h>
#include <bus/bus.h>
#include <device/driver.h>
#include <interface/block.h>

struct part_data {
    struct device *blkdev;
    const struct block_interface *blkif;
    lba_t part_base, part_limit;
};

static long read(struct device *dev, lba_t lba, void *buf, long count)
{
    struct part_data *data = (struct part_data *)dev->data;

    if (data->part_base + lba + count > data->part_limit) {
        count -= data->part_base + lba + count - data->part_limit;
    }

    if (count < 1) {
        return 0;
    }

    return data->blkif->read(data->blkdev, data->part_base + lba, buf, count);
}

static long write(struct device *dev, lba_t lba, const void *buf, long count)
{
    struct part_data *data = (struct part_data *)dev->data;

    if (data->part_base + lba + count > data->part_limit) {
        count -= data->part_base + lba + count - data->part_limit;
    }

    if (count < 1) {
        return 0;
    }

    return data->blkif->write(data->blkdev, data->part_base + lba, buf, count);
}

static const struct block_interface blkif = {
    .read = read,
    .write = write,
};

static int probe(struct device *dev);
static int remove(struct device *dev);
static const void *get_interface(struct device *dev, const char *name);

static struct device_driver drv = {
    .name = "part",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static int intlog10(int val)
{
    int ret = 0;
    while (val >= 10) {
        val /= 10;
        ret++;
    }
    return ret;
}

static int probe(struct device *dev)
{
    struct device *blkdev = dev->parent;
    if (!blkdev) return 1;
    const struct block_interface *blkif = blkdev->driver->get_interface(blkdev, "block");
    if (!blkif) return 1;
    
    if (!dev->resource) return 1;
    if (dev->resource->type != RT_LBA) return 1;
    if (dev->resource->base > dev->resource->limit) return 1;

    int name_len = strlen(blkdev->name) + intlog10(blkdev->id) + 1 + sizeof("p") + 1;
    char *name = mm_allocate(name_len);
    snprintf(name, name_len, "%s%dp", blkdev->name, blkdev->id);
    dev->name = name;
    dev->id = generate_device_id(dev->name);

    struct part_data *data = mm_allocate(sizeof(*data));
    data->blkdev = blkdev;
    data->blkif = blkif;
    data->part_base = dev->resource->base;
    data->part_limit = dev->resource->limit;

    dev->data = data;

    return 0;
}

static int remove(struct device *dev)
{
    return 0;
}

static const void *get_interface(struct device *dev, const char *name)
{
    if (strcmp(name, "block") == 0) {
        return &blkif;
    }

    return NULL;
}

DEVICE_DRIVER(drv)
