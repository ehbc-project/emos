#include <acpi/sdt.h>

int acpi_verify_table(void *table)
{
    uint8_t checksum = 0;
    for (int i = 0; i < ((struct acpi_sdt_header *)table)->length; i++) {
        checksum += ((uint8_t *)table)[i];
    }

    return !checksum;
}
