#ifndef __DEVICE_DEVICE_H__
#define __DEVICE_DEVICE_H__

#include <stdint.h>
#include <device/resource.h>

enum device_id_type {
    DIT_STRING = 0,
    DIT_PCI,
    DIT_USB,
};

struct device_id {
    enum device_id_type type;
    union {
        const char *string;
        struct {
            uint16_t vendor, device;
        } pci;
        struct {
            uint16_t vid, pid;
        } usb;
    };
};

struct device_ref {
    struct device_ref *next;
    struct device *dev;
};

struct device_driver;

struct device {
    struct device *next;
    struct device *next_bus;

    const char *name;
    const struct device_id *id;

    struct device_driver *driver;

    struct bus *bus;

    const struct device_ref *reference;
    struct resource *resource;
    void *data;
};

int register_device(struct device *dev);

struct device *find_device(const char *id);

struct device_ref *create_device_ref(struct device_ref *prev);

#endif // __DEVICE_DEVICE_H__
