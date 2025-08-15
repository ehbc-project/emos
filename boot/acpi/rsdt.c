#include <acpi/rsdt.h>

#include <stddef.h>
#include <string.h>

void *acpi_find_table(struct acpi_rsdt *rsdt, const char signature[4])
{
    for (int i = 0; i < (rsdt->header.length - sizeof(rsdt->header)) / sizeof(uint32_t); i++) {
        struct acpi_sdt_header *header = (struct acpi_sdt_header *)(rsdt->table_pointers[i]);
        if (strncmp(header->signature, signature, 4) == 0) {
            return (struct acpi_fadt *)header;
        }
    }

    return NULL;
}
