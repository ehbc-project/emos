#ifndef __INTERFACE_FLOPPY_H__
#define __INTERFACE_FLOPPY_H__

#include <device/device.h>

struct fdc_command {
    uint8_t send_size;
    uint8_t recv_size;
    uint8_t wait_irq;
    uint8_t command;
    uint8_t data[12];
};

struct floppy_interface {
    int (*reset)(struct device *, int);
    int (*send_command)(struct device *, int, struct fdc_command *);
    int (*send_command_dma_input)(struct device *, int, struct fdc_command *, void *, long);
    int (*send_command_dma_output)(struct device *, int, struct fdc_command *, const void *, long);
};

#endif // __INTERFACE_FLOPPY_H__
