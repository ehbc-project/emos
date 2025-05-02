#ifndef __I686_FARPTR_H__
#define __I686_FARPTR_H__

#include <stdint.h>

struct farptr {
    uint16_t offset;
    uint16_t segment;
} __attribute__((packed));

typedef struct farptr farptr_t;

#endif // __I686_FARPTR_H__