#include <device/driver.h>

#include <stddef.h>
#include <string.h>

#include <mm/mm.h>

static struct device *device_list_head = NULL;

int register_device(struct device *dev)
{
    if (!device_list_head) {
        device_list_head = dev;
    } else {
        struct device *current = device_list_head;
        while (current->next) {
            current = current->next;
        }
        current->next = dev;
    }

    if (!dev->driver) {
        return 1;
    }

    return dev->driver->probe(dev);
}

struct device *find_device(const char *id)
{
    struct device *current = device_list_head;
    
    while (current) {
        if (current->id->type != DIT_STRING) {
            goto next;
        }

        if (strcmp(current->id->string, id) != 0) {
            goto next;
        }

        return current;

next:
        current = current->next;
    }

    return NULL;
}

struct device_ref *create_device_ref(struct device_ref *prev)
{
    struct device_ref *next = mm_allocate(sizeof(*next));

    if (prev) {
        prev->next = next;
    }

    return next;
}
