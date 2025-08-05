#include "asm/pci/cfgspace.h"

#include "asm/io.h"

uint32_t _bus_pci_cfg_read(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, int size)
{
    uint32_t address = 0x80000000 | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);

    _i686_out32(PCI_CONFIG_ADDRESS, address);
    uint32_t val = _i686_in32(PCI_CONFIG_DATA);

    val >>= (offset & 0x03) << 3;
    val &= 0xFFFFFFFF >> (32 - (size << 3));
    return val;
}

void _bus_pci_cfg_write(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value, int size)
{
    uint32_t address = 0x80000000 | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);

    _i686_out32(PCI_CONFIG_ADDRESS, address);
    uint32_t val = _i686_in32(PCI_CONFIG_DATA);

    uint32_t mask = 0xFFFFFFFF >> (32 - (size << 3));
    val &= ~(mask << (offset & 0x03));
    val |= (value & mask) << (offset & 0x03);
    _i686_out32(PCI_CONFIG_DATA, val);
}
