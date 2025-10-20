#include <fs/driver.h>

#include <stddef.h>
#include <string.h>

#include <mm/mm.h>

static struct fs_driver *driver_list_head = NULL;

void register_fs_driver(struct fs_driver *drv)
{
    if (!driver_list_head) {
        driver_list_head = drv;
        return;
    }

    struct fs_driver *current = driver_list_head;
    while (current->next) {
        current = current->next;
    }

    current->next = drv;
    return;
}

struct fs_driver *find_fs_driver(const char *name)
{
    struct fs_driver *current = driver_list_head;
    while (current) {
        if (strcmp(name, current->name) == 0) {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

int fs_auto_mount(struct device *__restrict dev, const char *__restrict name)
{
    struct filesystem *fs = mm_allocate(sizeof(*fs));
    fs->dev = dev;
    
    struct fs_driver *current = driver_list_head;
    while (current) {
        fs->driver = current;
        if (!current->probe(fs)) {
            int err = current->mount(fs);
            if (err) return err;

            return register_filesystem(fs, name);
        }

        current = current->next;
    }
    fs->driver = NULL;

    return 1;
}
