#ifndef __DISK_DISK_H__
#define __DISK_DISK_H__

#include <stdint.h>

struct chs {
    int cylinder, head, sector;
};

typedef int64_t lba_t;

lba_t disk_chs_to_lba(struct chs chs, struct chs geom);

struct chs disk_lba_to_chs(lba_t lba, struct chs geom);

#endif // __DISK_DISK_H__
