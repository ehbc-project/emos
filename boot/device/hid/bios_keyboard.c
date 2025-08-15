#include <string.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <interface/char.h>

#include <asm/bios/keyboard.h>

static long read(struct device *dev, char *buf, unsigned long len)
{
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

static int probe(struct device *dev);
static int remove(struct device *dev);
static const void *get_interface(struct device *dev, const char *name);

static struct device_driver drv = {
    .name = "i8042",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static int probe(struct device *dev)
{
    struct device_id *current_id = mm_allocate(sizeof(*current_id));
    current_id->type = DIT_STRING;
    current_id->string = "kbd";
    dev->id = current_id;

    return 0;
}

static int remove(struct device *dev)
{
    return 0;
}

static const void *get_interface(struct device *dev, const char *name)
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
