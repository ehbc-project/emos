#include <device/resource.h>

#include <mm/mm.h>

struct resource *create_resource(struct resource *prev)
{
    struct resource *next = mm_allocate(sizeof(*next));

    if (prev) {
        prev->next = next;
    }

    return next;
}
