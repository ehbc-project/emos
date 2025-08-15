#ifndef __BUS_BUS_H__
#define __BUS_BUS_H__

struct bus {
    struct bus *next;

    const char *name;
    struct device *dev;
};

int register_bus(struct bus *bus);

#endif // __BUS_BUS_H__
