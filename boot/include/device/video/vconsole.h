#ifndef __DEVICE_VIDEO_VCONSOLE_H__
#define __DEVICE_VIDEO_VCONSOLE_H__

#include <stdint.h>

#include <device/device.h>
#include <interface/console.h>
#include <interface/char.h>
#include <interface/framebuffer.h>


struct vconsole_device {
    struct device dev;

    struct device *fbdev;
    const struct framebuffer_interface *fbdev_fbif;

    int width, height;
    int cursor_x, cursor_y;

    struct console_char_cell *char_buffer;
    uint8_t *diff_buffer;

    const struct console_interface *conif;
};

#endif // __DEVICE_VIDEO_VCONSOLE_H__
