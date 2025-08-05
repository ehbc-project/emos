#include <device/hid/bios_keyboard.h>

#include <string.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <interface/char.h>

#include <asm/bios/keyboard.h>

static long read(struct device *_dev, char *buf, unsigned long len)
{
    struct bios_keyboard_device *dev = (struct bios_keyboard_device *)_dev;

    int read_len = 0;
    while (read_len < len) {
        char ch;
        _pc_bios_read_keyboard(NULL, &ch);
        buf[read_len++] = ch;
    }

    return read_len;
}

static const struct char_interface charif = {
    .read = read,
};

static struct device *probe(const struct device_id *id);
static int remove(struct device *_dev);
static const void *get_interface(struct device *_dev, const char *name);

static struct device_driver drv = {
    .name = "bios_keyboard",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static struct device *probe(const struct device_id *id)
{
    struct bios_keyboard_device *dev = mm_allocate(sizeof(*dev));
    if (!dev) return NULL;
    dev->dev.driver = &drv;

    struct device_id *current_id = mm_allocate(sizeof(*current_id));
    current_id->type = DIT_STRING;
    current_id->string = "kbd";
    dev->dev.id = current_id;

    dev->charif = &charif;

    register_device((struct device *)dev);

    return (struct device *)dev;
}

static int remove(struct device *_dev)
{
    struct bios_keyboard_device *dev = (struct bios_keyboard_device *)_dev;

    mm_free(dev);

    return 0;
}

static const void *get_interface(struct device *_dev, const char *name)
{
    struct bios_keyboard_device *dev = (struct bios_keyboard_device *)_dev;

    if (strcmp(name, "char") == 0) {
        return dev->charif;
    }

    return NULL;
}

__attribute__((constructor))
static void _register_driver(void)
{
    register_driver((struct device_driver *)&drv);
}
