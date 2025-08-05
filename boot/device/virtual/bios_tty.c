#include <device/virtual/bios_tty.h>

#include <string.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <interface/char.h>

#include <asm/bios/video.h>

static long write(struct device *_dev, const char *buf, unsigned long len)
{
    struct bios_tty_device *dev = (struct bios_tty_device *)_dev;

    for (int i = 0; i < len; i++) {
        switch (buf[i]) {
            case '\t': {
                uint8_t row, col;
                _pc_bios_get_text_cursor(0, NULL, &row, &col);
                _pc_bios_set_text_cursor_pos(0, row, ((col / 8) + 1) * 8);
                break;
            }
            case '\x7F':
                _pc_bios_tty_output('\b');
                _pc_bios_tty_output('\0');
                _pc_bios_tty_output('\b');
                break;
            case '\n':
                _pc_bios_tty_output('\r');
            default:
                _pc_bios_tty_output(buf[i]);
        }
    }

    return 0;
}

static const struct char_interface charif = {
    .write = write,
};

static struct device *probe(const struct device_id *id);
static int remove(struct device *_dev);
static const void *get_interface(struct device *_dev, const char *name);

static struct device_driver drv = {
    .name = "bios_tty",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static struct device *probe(const struct device_id *id)
{
    struct bios_tty_device *dev = mm_allocate(sizeof(*dev));
    if (!dev) return NULL;
    dev->dev.driver = &drv;

    struct device_id *current_id = mm_allocate(sizeof(*current_id));
    current_id->type = DIT_STRING;
    current_id->string = "tty0";
    dev->dev.id = current_id;

    dev->charif = &charif;

    register_device((struct device *)dev);

    return (struct device *)dev;
}

static int remove(struct device *_dev)
{
    struct bios_tty_device *dev = (struct bios_tty_device *)_dev;

    mm_free(dev);

    return 0;
}

static const void *get_interface(struct device *_dev, const char *name)
{
    struct bios_tty_device *dev = (struct bios_tty_device *)_dev;

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
