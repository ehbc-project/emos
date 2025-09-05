#include <string.h>

#include <mm/mm.h>
#include <bus/bus.h>
#include <device/driver.h>
#include <interface/block.h>
#include <interface/ide.h>
#include <bus/ide/ata.h>

struct ata_data {
    struct device *idedev;
    const struct ide_interface *ideif;
    int slave;
};

static long read(struct device *dev, lba_t lba, void *buf, long count)
{
    struct ata_data *data = (struct ata_data *)dev->data;

    if (count > UINT16_MAX) count = UINT16_MAX;

    struct ata_command cmd;
    
    if (lba < (1 << 28)) {
        cmd = (struct ata_command){
            .extended = 0,
            .command = 0x20,
            .features = 0,
            .count = count,
            .sector = lba & 0xFF,
            .cylinder_low = (lba >> 8) & 0xFF,
            .cylinder_high = (lba >> 16) & 0xFF,
            .drive_head = 0x40 | (data->slave ? 0x10 : 0x00) | ((lba >> 24) & 0xF),
        };
    } else {
        cmd = (struct ata_command){
            .extended = 1,
            .command = 0x24,
            .features = 0,
            .count = count,
            .sector = (((lba >> 24) & 0xFF) << 8) | (lba & 0xFF),
            .cylinder_low = (((lba >> 32) & 0xFF) << 8) | ((lba >> 8) & 0xFF),
            .cylinder_high = (((lba >> 40) & 0xFF) << 8) | ((lba >> 16) & 0xFF),
            .drive_head = 0x40 | (data->slave ? 0x10 : 0x00),
        };
    }

    data->ideif->send_command_pio_input(data->idedev, &cmd, buf, 512 * count);

    return count;
}

static long write(struct device *dev, lba_t lba, const void *buf, long count)
{
    return count;
}

static const struct block_interface blkif = {
    .read = read,
    .write = write,
};

static int probe(struct device *dev);
static int remove(struct device *dev);
static const void *get_interface(struct device *dev, const char *name);

static struct device_driver drv = {
    .name = "ata",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static int probe(struct device *dev)
{
    if (!dev->bus) return 1;
    struct device *idedev = dev->parent;
    if (!idedev) return 1;
    const struct ide_interface *ideif = idedev->driver->get_interface(idedev, "ide");
    if (!ideif) return 1;

    if (!dev->resource) return 1;
    if (dev->resource->type != RT_BUS) return 1;
    if (dev->resource->base != dev->resource->limit) return 1;
    if (dev->resource->limit > 1) return 1;

    dev->name = "fd";
    dev->id = generate_device_id(dev->name);

    struct ata_data *data = mm_allocate(sizeof(*data));
    data->idedev = idedev;
    data->ideif = ideif;
    data->slave = dev->resource->base;

    dev->data = data;

    uint8_t idbuf[512];
    struct ata_device_ident *ata_ident = (struct ata_device_ident *)&idbuf;
    struct ata_command cmd = {
        .extended = 0,
        .command = 0xEC,  /* IDENTIFY DEVICE */
        .features = 0,
        .count = 0,
        .sector = 0,
        .cylinder_low = 0,
        .cylinder_high = 0,
        .drive_head = 0xA0 | (data->slave ? 0x10 : 0x00),
    };

    if (ideif->send_command_pio_input(idedev, &cmd, idbuf, sizeof(idbuf))) {
        return 1;
    }

    struct device *mbrdev = mm_allocate_clear(1, sizeof(struct device));
    mbrdev->parent = dev;
    mbrdev->driver = find_device_driver("mbr");
    register_device(mbrdev);

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

__attribute__((constructor))
static void _register_driver(void)
{
    register_device_driver(&drv);
}
