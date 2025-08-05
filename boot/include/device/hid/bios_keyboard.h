#ifndef __DEVICE_HID_BIOS_KEYBOARD_H__
#define __DEVICE_HID_BIOS_KEYBOARD_H__

#include <stdint.h>

#include <device/device.h>

struct bios_keyboard_device {
    struct device dev;

    const struct char_interface *charif;
};

#endif  // __DEVICE_HID_BIOS_KEYBOARD_H__
