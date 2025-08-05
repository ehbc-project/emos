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
    uint16_t vendor_id = _bus_pci_cfg_read(bus, device, function, PCI_CFGSPACE_VENDORID, sizeof(vendor_id));
    uint16_t device_id = _bus_pci_cfg_read(bus, device, function, PCI_CFGSPACE_DEVICEID, sizeof(device_id));
    uint8_t base_class = _bus_pci_cfg_read(bus, device, function, PCI_CFGSPACE_BASE_CLASS, sizeof(base_class));
    uint8_t sub_class = _bus_pci_cfg_read(bus, device, function, PCI_CFGSPACE_SUB_CLASS, sizeof(sub_class));
    uint8_t interface = _bus_pci_cfg_read(bus, device, function, PCI_CFGSPACE_INTERFACE, sizeof(interface));

    if (list->count >= list->max_count) {
        return -1;
    }

    list->devices[list->count] = (struct pci_device){
        .bus = bus,
        .device = device,
        .function = function,
        .vendor_id = vendor_id,
        .device_id = device_id,
        .base_class = base_class,
        .sub_class = sub_class,
        .interface = interface,
    };

    list->count++;

    return 0;
}

int _bus_pci_device_scan(struct pci_device_list *list, int bus, int device)
{
    _bus_pci_function_scan(list, bus, device, 0);

    uint8_t header_type = _bus_pci_cfg_read(bus, device, 0, PCI_CFGSPACE_HEADER_TYPE, sizeof(header_type));
    if (!(header_type & 0x80)) {
        return 0;
    }

    for (int function = 1; function < 8; function++) {
        uint16_t vendor_id = _bus_pci_cfg_read(bus, device, function, PCI_CFGSPACE_VENDORID, sizeof(vendor_id));
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
        uint16_t vendor_id = _bus_pci_cfg_read(bus, device, 0, PCI_CFGSPACE_VENDORID, sizeof(vendor_id));
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

    uint8_t header_type = _bus_pci_cfg_read(0, 0, 0, PCI_CFGSPACE_HEADER_TYPE, sizeof(header_type));

    if (!(header_type & 0x80)) {
        _bus_pci_bus_scan(&list, 0);
        return list.count;
    }

    for (int function = 1; function < 8; function++) {
        uint16_t vendor_id = _bus_pci_cfg_read(0, 0, 0, PCI_CFGSPACE_VENDORID, sizeof(vendor_id));
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
