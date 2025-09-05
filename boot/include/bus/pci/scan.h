#ifndef __BUS_PCI_SCAN_H__
#define __BUS_PCI_SCAN_H__

#include <bus/pci/device.h>

/**
 * @brief Scan for PCI devices on the host bus.
 * @param buf A pointer to a buffer to store the found PCI devices.
 * @param max_count The maximum number of PCI devices to store in the buffer.
 * @return The number of PCI devices found, or a negative error code.
 */
int pci_host_scan(struct pci_device *buf, int max_count);

#endif // __BUS_PCI_SCAN_H__
