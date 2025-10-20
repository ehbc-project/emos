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

struct device_driver;

struct device {
    struct device *next;
    struct device *sibling;
    struct device *parent;
    struct device *first_child;

    const char *name;
    int id;

    struct device_driver *driver;

    struct bus *bus;

    struct resource *resource;
    void *data;
};

int register_device(struct device *dev);
int unregister_device(struct device *dev);

struct device *get_first_device(void);
struct device *find_device(const char *id);

int generate_device_id(const char *name);

#endif // __DEVICE_DEVICE_H__
