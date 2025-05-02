#include "cfgspace.h"

struct pci_device {
    uint8_t bus, device, function;
    uint16_t vendor_id, device_id;
    struct pci_class_code class_code;
};
