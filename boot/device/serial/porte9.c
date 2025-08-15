#include <string.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <interface/char.h>

#include <asm/io.h>

static long write(struct device *_dev, const char *buf, unsigned long len)
{
    struct porte9_device *dev = (struct porte9_device *)_dev;

    for (int i = 0; i < len; i++) {
        _i686_out8(0xE9, buf[i]);
    }

    return len;
}

static const struct char_interface charif = {
    .write = write,
};

static int probe(struct device *dev);
static int remove(struct device *_dev);
static const void *get_interface(struct device *_dev, const char *name);

static struct device_driver drv = {
    .name = "porte9",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static int probe(struct device *dev)
{
    struct device_id *current_id = mm_allocate(sizeof(*current_id));
    current_id->type = DIT_STRING;
    current_id->string = "dbg";
    dev->id = current_id;

    return 0;
}

static int remove(struct device *_dev)
{
    return 0;
}

static const void *get_interface(struct device *_dev, const char *name)
{
    if (strcmp(name, "char") == 0) {
        return &charif;
    }

    return NULL;
}

__attribute__((constructor))
static void _register_driver(void)
{
    register_driver((struct device_driver *)&drv);
}
