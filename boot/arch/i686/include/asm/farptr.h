#ifndef __I686_FARPTR_H__
#define __I686_FARPTR_H__

#include <stdint.h>

struct farptr {
    uint16_t offset;
    uint16_t segment;
} __packed;

typedef struct farptr farptr_t;

#define FARPTR_TO_VPTR(type, far_ptr) (type *)((uint32_t)far_ptr.segment << 4 + far_ptr.offset);

#endif // __I686_FARPTR_H__