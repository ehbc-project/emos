#ifndef __INTERFACE_CHAR_H__
#define __INTERFACE_CHAR_H__

#include <device/device.h>

struct char_interface {
    long (*seek)(struct device *, long, int);
    long (*read)(struct device *, char *, long);
    long (*write)(struct device *, const char *, long);
    int (*flush)(struct device *);
};

#endif // __INTERFACE_CHAR_H__
