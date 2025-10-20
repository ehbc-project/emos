#include <string.h>
#include <stdio.h>

#include <mm/mm.h>
#include <bus/bus.h>
#include <device/driver.h>
#include <interface/block.h>

#include "gpt.h"

struct gpt_data {
    struct device *blkdev;
    const struct block_interface *blkif;
};

static long read(struct device *dev, lba_t lba, void *buf, long count)
{
    struct gpt_data *data = (struct gpt_data *)dev->data;

    return data->blkif->read(data->blkdev, lba, buf, count);
}

static long write(struct device *dev, lba_t lba, const void *buf, long count)
{
    struct gpt_data *data = (struct gpt_data *)dev->data;

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
    .name = "gpt",
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

    int name_len = strlen(blkdev->name) + intlog10(blkdev->id) + 1 + sizeof("pt") + 1;
    char *name = mm_allocate(name_len);
    snprintf(name, name_len, "%s%dpt", blkdev->name, blkdev->id);
    dev->name = name;
    dev->id = generate_device_id(dev->name);

    struct gpt_data *data = mm_allocate(sizeof(*data));
    data->blkdev = blkdev;
    data->blkif = blkif;

    dev->data = data;

    uint8_t buf[512];
    struct mbr *prot_mbr_sect = (struct mbr *)buf;
    data->blkif->read(data->blkdev, 0, buf, 1);

    if (prot_mbr_sect->boot_signature != MBR_SIGNATURE) return 1;
    if (prot_mbr_sect->partition_entries[0].type != MBR_PART_TYPE_GPT) return 1;

    lba_t gpt_base = prot_mbr_sect->partition_entries[0].base_lba;
    
    struct gpt_header *gpt_header = (struct gpt_header *)buf;
    data->blkif->read(data->blkdev, gpt_base, buf, 1);
    
    if (strncmp(gpt_header->signature, GPT_HEADER_SIGNATURE, sizeof(gpt_header->signature)) != 0) {
        return 1;
    }

    uint32_t bytes_per_entry = gpt_header->bytes_per_entry;
    uint32_t entry_count = gpt_header->entry_count;
    uint64_t entry_list_lba = gpt_header->entry_list_base_lba;
    
    uint32_t offset = 0;
    data->blkif->read(data->blkdev, entry_list_lba, buf, 1);

    static const gpt_guid null_entry = { 0, };
    for (int i = 0; i < entry_count; i++) {
        struct gpt_partition_entry *entry = (struct gpt_partition_entry *)&buf[offset];

        if (memcmp(entry->type, null_entry, sizeof(entry->type)) == 0) break;

        struct device *partdev = mm_allocate_clear(1, sizeof(struct device));
        partdev->parent = dev;
        struct resource *res = create_resource(NULL);
        res->type = RT_LBA;
        res->base = entry->base_lba;
        res->limit = entry->limit_lba;
        res->flags = 0;
        partdev->resource = res;
        partdev->driver = find_device_driver("part");
        register_device(partdev);

        offset += bytes_per_entry;
        if (offset >= 512) {
            offset -= 512;
            entry_list_lba++;
            data->blkif->read(data->blkdev, entry_list_lba, buf, 1);
        }
    }

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
