#ifndef __BUS_PCI_DRIVER_H__
#define __BUS_PCI_DRIVER_H__

#include <device/driver.h>

#include <bus/pci/id.h>

struct pci_device_driver {
    struct device_driver driver;

    struct pci_device_id id;
};

#endif // __BUS_PCI_DRIVER_H__
