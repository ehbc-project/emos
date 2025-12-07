#ifndef __EMOS_INTERFACE_WCHAR_H__
#define __EMOS_INTERFACE_WCHAR_H__

#include <wchar.h>

#include <emos/types.h>
#include <emos/device.h>
#include <emos/uuid.h>

#define WCHAR_INTERFACE_UUID UUID(0xD7, 0xBA, 0x42, 0xBF, 0x3F, 0xBD, 0x59, 0x74, 0x96, 0xFA, 0x38, 0x85, 0x3B, 0x24, 0x96, 0x63)

struct wchar_interface {
    status_t (*seek)(struct object *obj, offset_t offset, int whence, offset_t *result);
    status_t (*read)(struct object *obj, wchar_t *buf, size_t count, size_t *result);
    status_t (*write)(struct object *obj, const wchar_t *buf, size_t count, size_t *result);
};

#endif // __EMOS_INTERFACE_WCHAR_H__
