#ifndef __INTERFACE_IDE_H__
#define __INTERFACE_IDE_H__

#include <device/device.h>

struct ata_command {
    uint8_t extended;
    uint8_t command;
    uint16_t features;
    uint16_t count;
    uint16_t sector;
    uint16_t cylinder_low;
    uint16_t cylinder_high;
    uint8_t drive_head;
};

struct ide_interface {
    int (*send_command)(struct device *, struct ata_command *);
    int (*send_command_pio_input)(struct device *, struct ata_command *, void *, long, long);
    int (*send_command_pio_output)(struct device *, struct ata_command *, const void *, long, long);
};

#endif // __INTERFACE_IDE_H__
