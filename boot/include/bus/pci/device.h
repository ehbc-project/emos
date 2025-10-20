#ifndef __BUS_PCI_DEVICE_H__
#define __BUS_PCI_DEVICE_H__

#include <device/device.h>

#include <bus/pci/id.h>

struct pci_device {
    struct device dev;

    struct pci_device_id id;
};


#endif // __BUS_PCI_DEVICE_H__
