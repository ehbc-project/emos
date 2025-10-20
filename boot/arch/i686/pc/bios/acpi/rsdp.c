#include <uacpi/kernel_api.h>
#include <uacpi/acpi.h>

#include <stddef.h>

uacpi_status uacpi_kernel_get_rsdp(uacpi_phys_addr *out_rsdp_address) {
    static const void *rsdp_addr = NULL;

    if (!rsdp_addr) {
        uint16_t rsdp_base_seg = *(uint16_t *)0x40E;
        const char *ptr = (const char *)(rsdp_base_seg << 4);
    
        int match = 1;
    
        for (int i = 0; i < 8; i++) {
            if (ptr[i] != ACPI_RSDP_SIGNATURE[i]) {
                match = 0;
                break;
            }
        }
    
        for (uint32_t addr = 0x000E0000; addr <= 0x000FFFFF; addr++) {
            ptr = (const char *)addr;
            match = 1;
    
            for (int i = 0; i < 8; i++) {
                if (ptr[i] != ACPI_RSDP_SIGNATURE[i]) {
                    match = 0;
                    break;
                }
            }
    
            if (match) {
                rsdp_addr = (const void *)addr;
                break;
            }
        }
    }

    *out_rsdp_address = (uacpi_phys_addr)rsdp_addr;

    return rsdp_addr ? UACPI_STATUS_OK : UACPI_STATUS_NOT_FOUND;
}
