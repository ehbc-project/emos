#ifndef __DEVICE_VIRTUAL_BIOS_TTY_H__
#define __DEVICE_VIRTUAL_BIOS_TTY_H__

#include <stdint.h>

#include <device/device.h>

struct bios_tty_device {
    struct device dev;

    const struct char_interface *charif;
};

#endif  // __DEVICE_VIRTUAL_BIOS_TTY_H__