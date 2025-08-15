#include <acpi/rsdp.h>

#include <stddef.h>

int acpi_verify_rsdp(struct acpi_rsdp *rsdp)
{
    uint8_t checksum = 0;
    for (int i = 0; i < offsetof(struct acpi_rsdp, length); i++) {
        checksum += ((uint8_t *)rsdp)[i];
    }
    
    if (rsdp->revision < 2) return !checksum;

    checksum = 0;
    for (int i = offsetof(struct acpi_rsdp, length); i < sizeof(struct acpi_rsdp); i++) {
        checksum += ((uint8_t *)rsdp)[i];
    }

    return !checksum;
}
