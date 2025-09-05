
#ifndef __I686_PC_PCI_CFGSPACE_H__
#define __I686_PC_PCI_CFGSPACE_H__

#include <stdint.h>

#define PCI_CONFIG_ADDRESS 0x0CF8
#define PCI_CONFIG_DATA 0x0CFC

/**
 * @brief Read a 32-bit value from PCI configuration space.
 * @param bus The PCI bus number.
 * @param device The PCI device number.
 * @param function The PCI function number.
 * @param offset The offset within the configuration space.
 * @return The 32-bit value read.
 */
uint32_t _bus_pci_cfg_read32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

/**
 * @brief Read a 16-bit value from PCI configuration space.
 * @param bus The PCI bus number.
 * @param device The PCI device number.
 * @param function The PCI function number.
 * @param offset The offset within the configuration space.
 * @return The 16-bit value read.
 */
uint16_t _bus_pci_cfg_read16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

/**
 * @brief Read an 8-bit value from PCI configuration space.
 * @param bus The PCI bus number.
 * @param device The PCI device number.
 * @param function The PCI function number.
 * @param offset The offset within the configuration space.
 * @return The 8-bit value read.
 */
uint8_t _bus_pci_cfg_read8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

/**
 * @brief Write a 32-bit value to PCI configuration space.
 * @param bus The PCI bus number.
 * @param device The PCI device number.
 * @param function The PCI function number.
 * @param offset The offset within the configuration space.
 * @param value The 32-bit value to write.
 */
void _bus_pci_cfg_write32(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

/**
 * @brief Write a 16-bit value to PCI configuration space.
 * @param bus The PCI bus number.
 * @param device The PCI device number.
 * @param function The PCI function number.
 * @param offset The offset within the configuration space.
 * @param value The 16-bit value to write.
 */
void _bus_pci_cfg_write16(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);

/**
 * @brief Write an 8-bit value to PCI configuration space.
 * @param bus The PCI bus number.
 * @param device The PCI device number.
 * @param function The PCI function number.
 * @param offset The offset within the configuration space.
 * @param value The 8-bit value to write.
 */
void _bus_pci_cfg_write8(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t value);

#endif // __I686_PC_PCI_CFGSPACE_H__