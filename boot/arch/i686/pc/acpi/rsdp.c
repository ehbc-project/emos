#include "acpi/rsdp.h"

#include <stddef.h>

struct acpi_rsdp *acpi_find_rsdp(void) {

    uint16_t rsdp_base_seg = *(uint16_t*)0x40E;
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
            return (struct acpi_rsdp *)addr;
        }
    }

    return NULL;
}
