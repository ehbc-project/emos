#ifndef __EMOS_INTERFACE_BLOCK_H__
#define __EMOS_INTERFACE_BLOCK_H__

#include <emos/types.h>
#include <emos/device.h>
#include <emos/object.h>
#include <emos/uuid.h>

#define BLOCK_INTERFACE_UUID UUID(0xF8, 0xCA, 0x13, 0xD1, 0xDA, 0x6F, 0x5D, 0x57, 0xA2, 0xF0, 0x0B, 0xF7, 0xC4, 0x2C, 0xAB, 0xAA)

struct block_interface {
    status_t (*get_block_size)(struct object *obj, size_t *size);
    status_t (*fetch)(struct object *obj, lba_t lba, void **buf);
    void (*release)(struct object *obj, lba_t lba, int dirty);
    status_t (*flush)(struct object *obj, lba_t lba);
    status_t (*sync)(struct object *obj);
};

#endif // __EMOS_INTERFACE_BLOCK_H__
