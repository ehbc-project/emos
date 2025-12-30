#ifndef __EBOOT_INTERFACE_BLOCK_H__
#define __EBOOT_INTERFACE_BLOCK_H__

#include <eboot/status.h>
#include <eboot/device.h>
#include <eboot/disk.h>

#define BLOCK_INTERFACE_ID "block"

struct block_interface {
    status_t (*get_block_size)(struct device *, size_t *);
    status_t (*read)(struct device *, lba_t, void *, size_t, size_t *);
    status_t (*write)(struct device *, lba_t, const void *, size_t, size_t *);
};

#endif // __EBOOT_INTERFACE_BLOCK_H__
