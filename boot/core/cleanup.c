
#include <stddef.h>
#include <device/device.h>

extern void (*__fini_array_start)(void);
extern void (*__fini_array_end)(void);

void cleanup(void)
{
    struct device *dev = get_first_device();
    struct device *last_root_dev = NULL;

    while (dev) {
        if (!dev->parent) {
            if (last_root_dev) {
                unregister_device(last_root_dev);
            }

            last_root_dev = dev;
        }

        dev = dev->next;
    }

    if (last_root_dev) {
        unregister_device(last_root_dev);
    }

    for (int i = 0; &(&__fini_array_start)[i] != &__fini_array_end; i++) {
        (&__fini_array_start)[i]();
    }
}
