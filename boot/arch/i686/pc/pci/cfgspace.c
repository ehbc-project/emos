#include "cfgspace.h"

#include "../../io.h"

uint32_t _pc_pci_cfg_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset)
{
    uint32_t address = 0x80000000 | (bus << 16) | (device << 11) | (function << 8) | offset;

    _i686_out32(PCI_CONFIG_ADDRESS, address);
    return _i686_in32(PCI_CONFIG_DATA);
}

void _pc_pci_cfg_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value)
{
    uint32_t address = 0x80000000 | (bus << 16) | (device << 11) | (function << 8) | offset;

    _i686_out32(PCI_CONFIG_ADDRESS, address);
    _i686_out32(PCI_CONFIG_DATA, value);
}
