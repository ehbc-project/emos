#ifndef __DISK_DISK_H__
#define __DISK_DISK_H__

#include <stdint.h>

struct chs {
    int cylinder, head, sector;
};

typedef int64_t lba_t;

lba_t disk_chs_to_lba(struct chs chs);
struct chs disk_lba_to_chs(lba_t lba);

int disk_read(lba_t lba, void *buf, int count);
int disk_write(lba_t lba, const void *buf, int count);

#endif // __DISK_DISK_H__
