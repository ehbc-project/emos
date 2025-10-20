#include <string.h>
#include <stdio.h>

#include <mm/mm.h>
#include <bus/bus.h>
#include <device/driver.h>
#include <interface/block.h>

#include "mbr.h"

struct mbr_data {
    struct device *blkdev;
    const struct block_interface *blkif;
};

static long read(struct device *dev, lba_t lba, void *buf, long count)
{
    struct mbr_data *data = (struct mbr_data *)dev->data;

    return data->blkif->read(data->blkdev, lba, buf, count);
}

static long write(struct device *dev, lba_t lba, const void *buf, long count)
{
    struct mbr_data *data = (struct mbr_data *)dev->data;

    return data->blkif->write(data->blkdev, lba, buf, count);
}

static const struct block_interface blkif = {
    .read = read,
    .write = write,
};

static int probe(struct device *dev);
static int remove(struct device *dev);
static const void *get_interface(struct device *dev, const char *name);

static struct device_driver drv = {
    .name = "mbr",
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

static int register_partitions(struct device *dev, lba_t base)
{
    struct mbr_data *data = (struct mbr_data *)dev->data;

    uint8_t buf[512];
    struct mbr *mbr_sect = (struct mbr *)buf;
    data->blkif->read(data->blkdev, base, buf, 1);

    if (mbr_sect->boot_signature != MBR_SIGNATURE) return 1;
    if (mbr_sect->partition_entries[0].type == MBR_PART_TYPE_GPT) return 1;

    for (int i = 0; i < 4; i++) {
        struct mbr_partition_entry *entry = &mbr_sect->partition_entries[i];
        if (!entry->type) continue;

        if (entry->type == MBR_PART_TYPE_EXTENDED) {
            register_partitions(dev, base + entry->base_lba);
        } else {
            struct device *partdev = mm_allocate_clear(1, sizeof(struct device));
            partdev->parent = dev;
            struct resource *res = create_resource(NULL);
            res->type = RT_LBA;
            res->base = base + entry->base_lba;
            res->limit = base + entry->base_lba + entry->sector_count;
            res->flags = 0;
            partdev->resource = res;
            partdev->driver = find_device_driver("part");
            register_device(partdev);
        }
    }

    return 0;
}

static int probe(struct device *dev)
{
    struct device *blkdev = dev->parent;
    if (!blkdev) return 1;
    const struct block_interface *blkif = blkdev->driver->get_interface(blkdev, "block");
    if (!blkif) return 1;

    int name_len = strlen(blkdev->name) + intlog10(blkdev->id) + 1 + sizeof("pt") + 1;
    char *name = mm_allocate(name_len);
    snprintf(name, name_len, "%s%dpt", blkdev->name, blkdev->id);
    dev->name = name;
    dev->id = generate_device_id(dev->name);

    struct mbr_data *data = mm_allocate(sizeof(*data));
    data->blkdev = blkdev;
    data->blkif = blkif;

    dev->data = data;

    return register_partitions(dev, 0);
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
