#ifndef __DEVICE_RESOURCE_H__
#define __DEVICE_RESOURCE_H__

#include <stdint.h>

enum resource_type {
    RT_IOPORT,
    RT_MEMORY,
    RT_IRQ,
    RT_DMA,
    RT_BUS,
    RT_LBA,
};

struct resource {
    struct resource *next;

    enum resource_type type;
    uint64_t base;
    uint64_t limit;
    uint32_t flags;
};

/**
 * @brief Create a new resource structure.
 * @param prev A pointer to the previous resource in a linked list, or NULL if this is the first.
 * @return A pointer to the newly created resource.
 */
struct resource *create_resource(struct resource *prev);

#endif // __DEVICE_RESOURCE_H__
