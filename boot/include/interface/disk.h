#ifndef __INTERFACE_DISK_H__
#define __INTERFACE_DISK_H__

#include <device/device.h>
#include <disk/disk.h>

struct disk_interface {
    int (*get_geometry)(struct device *, struct chs *);
    long (*read)(struct device *, lba_t, void *, long);
    long (*write)(struct device *, lba_t, const void *, long);
};

#endif // __INTERFACE_DISK_H__
