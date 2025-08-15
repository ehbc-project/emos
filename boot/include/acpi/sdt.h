#ifndef __ACPI_SDT_H__
#define __ACPI_SDT_H__

#include <stdint.h>

struct acpi_sdt_header {
    char signature[4];
    uint32_t length;
    uint8_t revision;
    uint8_t checksum;
    char oem_id[6];
    char oem_table_id[8];
    uint32_t oem_revision;
    uint32_t creator_id;
    uint32_t creator_revision;
} __attribute__((packed));

int acpi_verify_table(void *table);

#endif // __ACPI_SDT_H__