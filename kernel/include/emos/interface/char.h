#ifndef __EMOS_INTERFACE_CHAR_H__
#define __EMOS_INTERFACE_CHAR_H__

#include <emos/types.h>
#include <emos/device.h>
#include <emos/uuid.h>

#define CHAR_INTERFACE_UUID UUID(0x6A, 0x90, 0x80, 0x7F, 0x31, 0xBC, 0x51, 0xED, 0x9F, 0xBD, 0x91, 0xAC, 0x54, 0x01, 0xBE, 0xDD)

struct char_interface {
    status_t (*seek)(struct object *obj, offset_t offset, int whence, offset_t *result);
    status_t (*read)(struct object *obj, char *buf, size_t count, size_t *result);
    status_t (*write)(struct object *obj, const char *buf, size_t count, size_t *result);
};

#endif // __EMOS_INTERFACE_CHAR_H__
