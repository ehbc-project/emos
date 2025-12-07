#ifndef __EMOS_UUID_H__
#define __EMOS_UUID_H__

#include <emos/types.h>
#include <emos/compiler.h>

#include <string.h>

typedef uint8_t uuid_t[16];

#define UUID(...) ((uuid_t){ __VA_ARGS__ })

static __always_inline int emos_uuid_isequal(uuid_t uuid1, uuid_t uuid2)
{
    return memcmp(uuid1, uuid2, sizeof(uuid1)) == 0;
}

#endif // __EMOS_UUID_H__
