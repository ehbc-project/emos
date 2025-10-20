#ifndef __BUS_PCI_ID_H__
#define __BUS_PCI_ID_H__

#include <stdint.h>

#define PCI_DEVICE_ID_ANY   0xFFFFFFFF

struct pci_device_id {
    uint32_t vendor_id, device_id;
    uint32_t base_class, sub_class, interface;
};

struct pci_device_driver;

int _bus_pci_match_id(const struct pci_device_driver *driver, uint16_t vendor_id, uint16_t device_id, uint16_t base_class, uint16_t sub_class, uint16_t interface);

#endif // __BUS_PCI_ID_H__
