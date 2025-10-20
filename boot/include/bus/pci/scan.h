#ifndef __BUS_PCI_SCAN_H__
#define __BUS_PCI_SCAN_H__

#include <bus/pci/device.h>

int pci_host_scan(struct pci_device *buf, int max_count);

#endif // __BUS_PCI_SCAN_H__
