#ifndef __EBOOT_INTERFACE_HID_H__
#define __EBOOT_INTERFACE_HID_H__

#include <eboot/status.h>
#include <eboot/device.h>

struct hid_interface {
    status_t (*wait_event)(struct device *);
    status_t (*poll_event)(struct device *, uint16_t *, uint16_t *);
};

#endif // __EBOOT_INTERFACE_HID_H__
