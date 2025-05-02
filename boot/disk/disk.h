#ifndef __DISK_H__
#define __DISK_H__

#include <stdint.h>

struct chs {
    int cylinder, head, sector;
};

typedef int64_t lba_t;

#endif // __DISK_H__
