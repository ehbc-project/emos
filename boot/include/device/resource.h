#ifndef __DEVICE_RESOURCE_H__
#define __DEVICE_RESOURCE_H__

#include <stdint.h>

enum resource_type {
    RT_IOPORT,
    RT_MEMORY,
    RT_IRQ,
    RT_DMA,
};

struct resource {
    struct resource *next;

    enum resource_type type;
    uint64_t base;
    uint64_t limit;
    uint32_t flags;
};

struct resource *create_resource(struct resource *prev);

#endif // __DEVICE_RESOURCE_H__
