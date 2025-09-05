#include <string.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <fs/driver.h>
#include <interface/block.h>
#include <disk/disk.h>

#include <asm/io.h>

#include "afs.h"

struct afs_data {
    struct device *blkdev;
    const struct block_interface *blkif;

    uint16_t    reserved_sectors;
    uint8_t     sectors_per_block;
    uint16_t    sector_size;
    uint32_t    block_size;
    uint64_t    udb_pointer;
    uint64_t    jbb_pointer;
    uint64_t    rbb_pointer;
    uint64_t    group0_gbb_pointer;
    uint64_t    root_mdb_pointer;

    int64_t     blkbuf_num;
    uint8_t     *blkbuf;
};

struct afs_dir_data {
    uint64_t    block;
    uint16_t    offset;
};

static int block_to_sector(struct filesystem *fs, lba_t *lba, uint64_t block)
{
    struct afs_data *data = (struct afs_data *)fs->data;

    return data->reserved_sectors + block * data->sectors_per_block;
}

static int read_block(struct filesystem *fs, uint64_t block)
{
    struct afs_data *data = (struct afs_data *)fs->data;

    lba_t lba;
    block_to_sector(fs, &lba, block);
    if (data->blkbuf_num == lba) return 0;

    data->blkbuf_num = block;
    return data->blkif->read(data->blkdev, lba, data->blkbuf, data->sectors_per_block);
}

static int probe(struct filesystem *fs);
static int mount(struct filesystem *fs);
static void unmount(struct filesystem *fs);

static struct fs_file *open(struct fs_directory *dir, const char *name);
static long read(struct fs_file *file, void *buf, long len);
static int seek(struct fs_file *file, long offset, int origin);
static long tell(struct fs_file *file);
static void close(struct fs_file *file);

static struct fs_directory *open_root_directory(struct filesystem *fs);
static struct fs_directory *open_directory(struct fs_directory *dir, const char *name);
static int rewind_directory(struct fs_directory *dir);
static int iter_directory(struct fs_directory *dir, struct fs_directory_entry *entry);
static void close_directory(struct fs_directory *dir);

static struct fs_driver drv = {
    .name = "afs",
    .probe = probe,
    .mount = mount,
    .unmount = unmount,
    .open = open,
    .read = read,
    .seek = seek,
    .tell = tell,
    .close = close,
    .open_root_directory = open_root_directory,
    .open_directory = open_directory,
    .rewind_directory = rewind_directory,
    .iter_directory = iter_directory,
    .close_directory = close_directory,
};

static int probe(struct filesystem *fs)
{
    struct device *blkdev = fs->dev;
    if (!blkdev) return 1;
    const struct block_interface *blkif = blkdev->driver->get_interface(blkdev, "block");
    if (!blkif) return 1;

    uint8_t buf[512];

    /* read sector 0 */
    blkif->read(blkdev, 0, buf, 1);
    const struct afs_first_sector *lba0 = (const struct afs_first_sector *)buf;

    /* check signatures */
    if (lba0->vbr_signature != 0xAA55) {
        return 1;
    }
    if ((strncmp(lba0->filesystem_signature, "AFS\0", sizeof(lba0->filesystem_signature)) != 0)) {
        return 1;
    }

    return 0;
}

static int mount(struct filesystem *fs)
{
    struct device *blkdev = fs->dev;
    if (!blkdev) return 1;
    const struct block_interface *blkif = blkdev->driver->get_interface(blkdev, "block");
    if (!blkif) return 1;

    struct afs_data *data = mm_allocate(sizeof(*data));
    data->blkdev = blkdev;
    data->blkif = blkif;
    data->blkbuf_num = -1;

    fs->data = data;

    uint8_t buf[512];

    /* read sector 0 */
    blkif->read(blkdev, 0, buf, 1);
    const struct afs_first_sector *lba0 = (const struct afs_first_sector *)buf;

    /* check signatures */
    if (lba0->vbr_signature != 0xAA55) {
        return 1;
    }
    if ((strncmp(lba0->filesystem_signature, "AFS\0", sizeof(lba0->filesystem_signature)) != 0)) {
        return 1;
    }

    data->reserved_sectors = lba0->reserved_sectors;

    /* read the first sector of RDB */
    read_block(fs, 0);
    const struct afs_rdb *rdb = (const struct afs_rdb *)buf;

    data->sectors_per_block = rdb->sectors_per_block;
    data->sector_size = rdb->bytes_per_sector;
    data->block_size = data->sector_size * rdb->sectors_per_block;
    data->blkbuf = mm_allocate(data->block_size);

    /* read entire RDB */
    blkif->read(blkdev, lba0->reserved_sectors, data->blkbuf, data->sectors_per_block);
    data->blkbuf_num = 0;
    rdb = (const struct afs_rdb *)data->blkbuf;

    data->udb_pointer = rdb->udb_pointer;
    data->jbb_pointer = rdb->jbb_pointer;
    data->rbb_pointer = rdb->rbb_pointer;
    data->group0_gbb_pointer = rdb->group0_gbb_pointer;
    data->root_mdb_pointer = rdb->root_mdb_pointer;

    return 0;
}

static void unmount(struct filesystem *fs)
{
    
}

static struct fs_file *open(struct fs_directory *dir, const char *name)
{
    return NULL;
}

static long read(struct fs_file *file, void *buf, long len)
{
    return -1;
}

static int seek(struct fs_file *file, long offset, int origin)
{
    return 1;
}

static long tell(struct fs_file *file)
{
    return -1;
}

static void close(struct fs_file *file)
{

}

static struct fs_directory *open_root_directory(struct filesystem *fs)
{
    struct afs_data *data = (struct afs_data *)fs->data;

    struct afs_dir_data *dir_data = mm_allocate(sizeof(*dir_data));
    dir_data->block = data->root_mdb_pointer;
    dir_data->offset = offsetof(struct afs_acb, entries);
    
    struct fs_directory *dir = mm_allocate(sizeof(*dir));
    dir->fs = fs;
    dir->data = dir_data;

    return dir;
}

static struct fs_directory *open_directory(struct fs_directory *dir, const char *name)
{
    struct filesystem *fs = dir->fs;
    struct afs_data *data = (struct afs_data *)fs->data;
    struct afs_dir_data *dir_data = (struct afs_dir_data *)dir->data;

    struct afs_dir_data *new_dir_data = mm_allocate(sizeof(*new_dir_data));
    
    struct fs_directory *new_dir = mm_allocate(sizeof(*dir));
    new_dir->fs = fs;
    new_dir->data = new_dir_data;

    return new_dir;
}

static int rewind_directory(struct fs_directory *dir)
{
    return 1;
}

static int iter_directory(struct fs_directory *dir, struct fs_directory_entry *entry)
{
    return 1;
}

static void close_directory(struct fs_directory *dir)
{
    
}

__attribute__((constructor))
static void _register_driver(void)
{
    register_fs_driver(&drv);
}
