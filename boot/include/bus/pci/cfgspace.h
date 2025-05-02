
#ifndef __BUS_PCI_CFGSPACE_H__
#define __BUS_PCI_CFGSPACE_H__

#include <stdint.h>

struct pci_class_code {
    uint8_t interface;
    uint8_t sub_class;
    uint8_t base_class;
};

struct pci_cfg_space_header {
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t command;
    uint32_t status;
    uint8_t revision_id;
    struct pci_class_code class_code;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
    uint32_t bar[6];
    uint32_t cardbus_cis_pointer;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base_address;
    uint8_t capabilities_pointer;
    uint8_t reserved1[3];
    uint32_t reserved2;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_grant;
    uint8_t max_latency;
};

#endif // __BUS_PCI_CFGSPACE_H__