#include "scan.h"

#include "cfgspace.h"

#include "../bios/video.h"

static void print_hex_byte(uint8_t value)
{
    static const char hex_chars[] = "0123456789ABCDEF";
    _pc_bios_tty_output(hex_chars[value >> 4]);
    _pc_bios_tty_output(hex_chars[value & 0xF]);
}

static void print_hex_word(uint16_t value)
{
    print_hex_byte(value >> 8);
    _pc_bios_tty_output(' ');
    print_hex_byte(value);
}

static void print_hex_dword(uint32_t value)
{
    print_hex_byte(value >> 24);
    _pc_bios_tty_output(' ');
    print_hex_byte(value >> 16);
    _pc_bios_tty_output(' ');
    print_hex_byte(value >> 8);
    _pc_bios_tty_output(' ');
    print_hex_byte(value);
}

int _pc_pci_function_scan(struct pci_device *buf, int max_count, int bus, int device, int function)
{
    uint16_t vendor_id = _pc_pci_cfg_read(bus, device, function, 0x00) & 0xFFFF;
    uint16_t device_id = _pc_pci_cfg_read(bus, device, function, 0x00) >> 16;
    uint32_t class_code = _pc_pci_cfg_read(bus, device, function, 0x08);
    uint8_t base_class = (class_code >> 16) & 0xFF;
    uint8_t sub_class = (class_code >> 8) & 0xFF;
    uint8_t interface_id = class_code & 0xFF;

    print_hex_byte(bus);
    _pc_bios_tty_output(' ');
    print_hex_byte(device);
    _pc_bios_tty_output(' ');
    print_hex_byte(function);
    _pc_bios_tty_output(':');
    print_hex_word(vendor_id);
    _pc_bios_tty_output(' ');
    print_hex_word(device_id);
    _pc_bios_tty_output(' ');
    print_hex_byte(base_class);
    _pc_bios_tty_output(' ');
    print_hex_byte(sub_class);
    _pc_bios_tty_output(' ');
    print_hex_byte(interface_id);
    _pc_bios_tty_output('\r');
    _pc_bios_tty_output('\n');

    return 0;
}

int _pc_pci_device_scan(struct pci_device *buf, int max_count, int bus, int device)
{
    _pc_pci_function_scan(buf, max_count, bus, device, 0);

    uint32_t header_type = (_pc_pci_cfg_read(bus, device, 0, 0x0C) & 0x00FF0000) >> 16;
    if (!(header_type & 0x80)) {
        return 0;
    }

    for (int function = 1; function < 8; function++) {
        uint32_t vendor_id = _pc_pci_cfg_read(bus, device, function, 0x00) & 0xFFFF;
        if (vendor_id == 0xFFFF) {
            continue;
        }
        
        int err = _pc_pci_function_scan(buf, max_count, bus, device, function);
        if (err) {
            return err;
        }
    }

    return 0;
}

int _pc_pci_bus_scan(struct pci_device *buf, int max_count, int bus)
{
    for (int device = 0; device < 32; device++) {
        uint32_t vendor_id = _pc_pci_cfg_read(bus, device, 0, 0x00) & 0xFFFF;
        if (vendor_id == 0xFFFF) {
            continue;
        }
        
        int err = _pc_pci_device_scan(buf, max_count, bus, device);
        if (err) {
            return err;
        }
    }

    return 0;
}

int _pc_pci_host_scan(struct pci_device *buf, int max_count)
{
    uint32_t header_type = (_pc_pci_cfg_read(0, 0, 0, 0x0C) & 0x00FF0000) >> 16;

    if (!(header_type & 0x80)) {
        return _pc_pci_bus_scan(buf, max_count, 0);
    }

    for (int function = 1; function < 8; function++) {
        uint32_t vendor_id = _pc_pci_cfg_read(0, 0, function, 0x00) & 0xFFFF;
        if (vendor_id == 0xFFFF) {
            continue;
        }

        int err = _pc_pci_function_scan(buf, max_count, 0, 0, function);
        if (err) {
            return err;
        }
    }

    return 0;
}
