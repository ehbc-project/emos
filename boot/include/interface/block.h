#ifndef __INTERFACE_BLOCK_H__
#define __INTERFACE_BLOCK_H__

#include <device/device.h>
#include <disk/disk.h>

struct block_interface {
    long (*read)(struct device *, lba_t, void *, long);
    long (*write)(struct device *, lba_t, const void *, long);
};

#endif // __INTERFACE_BLOCK_H__
