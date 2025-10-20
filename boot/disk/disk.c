#include <disk/disk.h>


lba_t disk_chs_to_lba(struct chs chs, struct chs geom)
{
    return (chs.cylinder * geom.head + chs.head) * geom.sector + chs.sector - 1;
}

struct chs disk_lba_to_chs(lba_t lba, struct chs geom)
{
    struct chs chs;
    chs.sector = (lba % geom.sector) + 1;
    chs.head = (lba / geom.sector) % geom.head;
    chs.cylinder = lba / geom.sector / geom.head;

    return chs;
}
