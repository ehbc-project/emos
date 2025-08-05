#include <device/driver.h>

#include <stddef.h>
#include <string.h>

static struct device_driver *driver_list_head = NULL;

void register_driver(struct device_driver *drv)
{
    if (!driver_list_head) {
        driver_list_head = drv;
        return;
    }

    struct device_driver *current = driver_list_head;
    while (current->next) {
        current = current->next;
    }

    current->next = drv;
    return;
}

struct device_driver *find_driver(const char *name)
{
    struct device_driver *current = driver_list_head;
    while (current) {
        if (strcmp(name, current->name) == 0) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

