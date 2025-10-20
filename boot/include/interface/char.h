#ifndef __INTERFACE_CHAR_H__
#define __INTERFACE_CHAR_H__

#include <device/device.h>

#include <wchar.h>

struct char_interface {
    long (*seek)(struct device *, long, int);
    long (*read)(struct device *, char *, long);
    long (*write)(struct device *, const char *, long);
    int (*flush)(struct device *);
};

struct wchar_interface {
    long (*seek)(struct device *, long, int);
    long (*read)(struct device *, wchar_t *, long);
    long (*write)(struct device *, const wchar_t *, long);
    int (*flush)(struct device *);
};

#endif // __INTERFACE_CHAR_H__
