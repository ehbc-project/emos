#ifndef __INTERFACE_CHAR_H__
#define __INTERFACE_CHAR_H__

#include <device/device.h>

struct char_interface {
    long (*seek)(struct device *, long, int);
    long (*read)(struct device *, char *, unsigned long);
    long (*write)(struct device *, const char *, unsigned long);
};

#endif // __INTERFACE_CHAR_H__
