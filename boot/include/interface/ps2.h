#ifndef __INTERFACE_PS2_H__
#define __INTERFACE_PS2_H__

#include <device/device.h>

struct ps2_interface {
    void (*enable_port)(struct device *dev, int port);
    void (*disable_port)(struct device *dev, int port);
    int (*test_port)(struct device *dev, int port);
    int (*send_data)(struct device *dev, int port, const uint8_t *data, int len);
    int (*recv_data)(struct device *dev, int port, uint8_t *data, int len);
    uint8_t (*irq_get_byte)(struct device *dev);
};

#endif // __INTERFACE_PS2_H__
