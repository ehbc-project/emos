#include "bus/pci/scan.h"

#include "asm/pci/cfgspace.h"

#include "bus/pci/device.h"

struct pci_device_list {
    struct pci_device *devices;
    int count;
    int max_count;
};

int _bus_pci_function_scan(struct pci_device_list *list, int bus, int device, int function)
{
    uint16_t vendor_id = _bus_pci_cfg_read(bus, device, function, 0x00) & 0xFFFF;
    uint16_t device_id = _bus_pci_cfg_read(bus, device, function, 0x00) >> 16;
    uint32_t class_code = _bus_pci_cfg_read(bus, device, function, 0x08);
    uint8_t base_class = (class_code >> 24) & 0xFF;
    uint8_t sub_class = (class_code >> 16) & 0xFF;
    uint8_t interface_id = (class_code >> 8) & 0xFF;

    if (list->count >= list->max_count) {
        return -1;
    }

    list->devices[list->count].bus = bus;
    list->devices[list->count].device = device;
    list->devices[list->count].function = function;
    list->devices[list->count].vendor_id = vendor_id;
    list->devices[list->count].device_id = device_id;
    list->devices[list->count].class_code = (struct pci_class_code){
        .base_class = base_class,
        .sub_class = sub_class,
        .interface = interface_id
    };

    list->count++;

    return 0;
}

int _bus_pci_device_scan(struct pci_device_list *list, int bus, int device)
{
    _bus_pci_function_scan(list, bus, device, 0);

    uint32_t header_type = (_bus_pci_cfg_read(bus, device, 0, 0x0C) & 0x00FF0000) >> 16;
    if (!(header_type & 0x80)) {
        return 0;
    }

    for (int function = 1; function < 8; function++) {
        uint32_t vendor_id = _bus_pci_cfg_read(bus, device, function, 0x00) & 0xFFFF;
        if (vendor_id == 0xFFFF) {
            continue;
        }
        
        int err = _bus_pci_function_scan(list, bus, device, function);
        if (err) {
            return err;
        }
    }

    return 0;
}

int _bus_pci_bus_scan(struct pci_device_list *list, int bus)
{
    for (int device = 0; device < 32; device++) {
        uint32_t vendor_id = _bus_pci_cfg_read(bus, device, 0, 0x00) & 0xFFFF;
        if (vendor_id == 0xFFFF) {
            continue;
        }
        
        int err = _bus_pci_device_scan(list, bus, device);
        if (err) {
            return err;
        }
    }

    return 0;
}

int pci_host_scan(struct pci_device *buf, int max_count)
{
    struct pci_device_list list;
    list.devices = buf;
    list.count = 0;
    list.max_count = max_count;

    uint32_t header_type = (_bus_pci_cfg_read(0, 0, 0, 0x0C) & 0x00FF0000) >> 16;

    if (!(header_type & 0x80)) {
        _bus_pci_bus_scan(&list, 0);
        return list.count;
    }

    for (int function = 1; function < 8; function++) {
        uint32_t vendor_id = _bus_pci_cfg_read(0, 0, function, 0x00) & 0xFFFF;
        if (vendor_id == 0xFFFF) {
            continue;
        }

        int err = _bus_pci_function_scan(&list, 0, 0, function);
        if (err) {
            return err;
        }
    }

    return list.count;
}
