#include <device/driver.h>

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <mm/mm.h>

static struct device *device_list_head = NULL;

struct device *get_first_device(void)
{
    return device_list_head;
}

int register_device(struct device *dev)
{
    if (!dev->driver) {
        return 1;
    }

    int err = dev->driver->probe(dev);
    if (err) {
        return err;
    }

    if (!device_list_head) {
        device_list_head = dev;
    } else {
        struct device *current = device_list_head;
        while (current->next) {
            current = current->next;
        }
        current->next = dev;
    }

    return 0;
}

struct device *find_device(const char *id)
{
    int name_len = strlen(id), num;
    for (int i = name_len; i > 0 && isdigit(id[i - 1]); i--) {
        name_len--;
    }
    num = strtol(id + name_len, NULL, 10);

    struct device *current = device_list_head;
    
    while (current) {
        if (strncmp(current->name, id, name_len) != 0 || current->name[name_len] || current->id != num) {
            current = current->next;
            continue;
        }

        return current;
    }

    return NULL;
}

int generate_device_id(const char *name)
{
    struct device *current = device_list_head;
    int id_max = -1;
    
    while (current) {
        if (strcmp(current->name, name) == 0 && id_max < current->id) {
            id_max = current->id;
        }
        current = current->next;
    }

    return id_max + 1;
}
