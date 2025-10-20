#ifndef __BUS_BUS_H__
#define __BUS_BUS_H__

struct bus {
    struct bus *next;

    const char *name;
    struct device *dev;
};

int register_bus(struct bus *bus);

#endif // __BUS_BUS_H__

/*
#ifndef __BUS_BUS_H__
#define __BUS_BUS_H__

#include <device/device.h>
#include <device/driver.h>

struct bus_type {
    struct bus_type *next;

    const char *name;
    struct device *dev;

    int (*match)(struct device *dev, struct driver *driver);
    int (*probe)(struct device *dev, struct driver *driver);
    void (*scan)(struct device *parent);
};

int register_bus_type(struct bus *bus);

#endif // __BUS_BUS_H__

*/
