#ifndef __DISK_DISK_H__
#define __DISK_DISK_H__

#include <stdint.h>

struct chs {
    int cylinder, head, sector;
};

typedef int64_t lba_t;

/**
 * @brief Convert CHS (Cylinder-Head-Sector) address to LBA (Logical Block Addressing).
 * @param chs The CHS address to convert.
 * @return The corresponding LBA address.
 */
lba_t disk_chs_to_lba(struct chs chs);

/**
 * @brief Convert LBA (Logical Block Addressing) address to CHS (Cylinder-Head-Sector).
 * @param lba The LBA address to convert.
 * @return The corresponding CHS address.
 */
struct chs disk_lba_to_chs(lba_t lba);

#endif // __DISK_DISK_H__
