#ifndef __BUS_BUS_H__
#define __BUS_BUS_H__

struct bus {
    struct bus *next;

    const char *name;
    struct device *dev;
};

/**
 * @brief Register a bus.
 * @param bus A pointer to the bus structure to register.
 * @return 0 on success, otherwise an error code.
 */
int register_bus(struct bus *bus);

#endif // __BUS_BUS_H__
