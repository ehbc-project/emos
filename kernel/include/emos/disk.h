#ifndef __EMOS_DISK_H__
#define __EMOS_DISK_H__

#include <emos/types.h>

struct chs {
    int cylinder, head, sector;
};

lba_t disk_chs_to_lba(struct chs chs, struct chs geom);

struct chs disk_lba_to_chs(lba_t lba, struct chs geom);

#endif // __EMOS_DISK_H__
