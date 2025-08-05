#ifndef __BUS_PCI_DEVICE_H__
#define __BUS_PCI_DEVICE_H__

#include "bus/pci/cfgspace.h"

struct pci_device {
    uint8_t bus, device, function;
    uint16_t vendor_id, device_id;
    uint8_t base_class, sub_class, interface;
};


#endif // __BUS_PCI_DEVICE_H__
