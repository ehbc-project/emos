#ifndef __INTERFACE_HID_H__
#define __INTERFACE_HID_H__

#include <device/device.h>

#include <hid/hid.h>

struct hid_interface {
    int (*wait_event)(struct device *);
    int (*poll_event)(struct device *, uint16_t *key, uint16_t *flags);
};

#endif // __INTERFACE_HID_H__
