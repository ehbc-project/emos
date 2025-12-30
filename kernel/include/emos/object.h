#ifndef __EMOS_OBJECT_H__
#define __EMOS_OBJECT_H__

#include <emos/uuid.h>

#define CHECK_OBJECT_TYPE(o, t) ((o)->type == (t))

#define OBJECT(o) ((struct object *)o)

#define OT_DEVICE       0x00000000
#define OT_FILESYSTEM   0x00000001
#define OT_BUS          0x00000002
#define OT_VOLUME       0x00000003

struct object {
    uint32_t type;
};

#endif // __EMOS_OBJECT_H__
