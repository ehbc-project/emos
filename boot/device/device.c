#include <device/driver.h>

#include <stddef.h>
#include <string.h>

#include <mm/mm.h>

struct device_list {
    struct device_list *next;
    struct device *dev;
};

static struct device_list *device_list = NULL;

int register_device(struct device *dev)
{
    struct device_list **last_elem = NULL;
    if (!device_list) {
        last_elem = &device_list;
    } else {
        struct device_list *current = device_list;
        while (current->next) {
            current = current->next;
        }
        last_elem = &current->next;
    }

    *last_elem = mm_allocate(sizeof(struct device_list));
    (*last_elem)->next = NULL;
    (*last_elem)->dev = dev;

    return 0;
}

struct device *find_device(const char *id)
{
    struct device_list *current = device_list;

    if (!current) {
        return NULL;
    }

    do {
        if (current->dev->id->type != DIT_STRING) {
            goto next;
        }

        if (strcmp(current->dev->id->string, id) != 0) {
            goto next;
        }

        return current->dev;

next:
        current = current->next;
    } while (current);

    return NULL;
}
