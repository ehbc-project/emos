
#ifndef __I686_PC_PCI_CFGSPACE_H__
#define __I686_PC_PCI_CFGSPACE_H__

#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0x0CF8
#define PCI_CONFIG_DATA 0x0CFC

uint32_t _bus_pci_cfg_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void _bus_pci_cfg_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

#endif // __I686_PC_PCI_CFGSPACE_H__