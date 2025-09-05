#include <string.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <fs/driver.h>
#include <interface/char.h>

#include <asm/io.h>

struct iso9660_data {
    int a;
};

static int probe(struct filesystem *fs);
static int mount(struct filesystem *fs);
static void unmount(struct filesystem *fs);

static struct fs_driver drv = {
    .name = "iso9660",
    .probe = probe,
    .mount = mount,
    .unmount = unmount,
};

static int probe(struct filesystem *fs)
{
    struct device *blkdev = fs->dev;
    if (!blkdev) return 1;
    const struct block_interface *blkdev_blkif = blkdev->driver->get_interface(blkdev, "block");
    if (!blkdev_blkif) return 1;

    return 1;
}

static int mount(struct filesystem *fs)
{
    struct device *blkdev = fs->dev;
    if (!blkdev) return 1;
    const struct block_interface *blkdev_blkif = blkdev->driver->get_interface(blkdev, "block");
    if (!blkdev_blkif) return 1;

    struct iso9660_data *data = mm_allocate(sizeof(*data));

    fs->data = data;

    return 1;
}

static void unmount(struct filesystem *fs)
{
    
}

__attribute__((constructor))
static void _register_driver(void)
{
    register_fs_driver(&drv);
}
