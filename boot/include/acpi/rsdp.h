#ifndef __ACPI_RSDP_H__
#define __ACPI_RSDP_H__

#include <stdint.h>

#define ACPI_RSDP_SIGNATURE "RSD PTR "

struct acpi_rsdp {
    /* ACPI 1.0 */
    char signature[8];
    uint8_t checksum;
    char oem_id[6];
    uint8_t revision;
    uint32_t rsdt_address;

    /* ACPI 2.0 */
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    char reserved[3];
} __attribute__((packed));

struct acpi_rsdp *acpi_rsdp_find(void);

int acpi_rsdp_verify(struct acpi_rsdp *rsdp);

#endif // __ACPI_RSDP_H__
