#ifndef __INTERFACE_BLOCK_H__
#define __INTERFACE_BLOCK_H__

#include <device/device.h>
#include <disk/disk.h>

struct block_interface {
    long (*seek)(struct device *, long, int);
    int (*read)(struct disk_device *, lba_t, void *, int);
    int (*write)(struct disk_device *, lba_t, const void *, int);
};

#endif // __INTERFACE_BLOCK_H__
