#ifndef __ACPI_RSDT_H__
#define __ACPI_RSDT_H__

#include <stdint.h>

#include "acpi/sdt.h"

#define ACPI_RSDT_SIGNATURE "RSDT"

struct acpi_rsdt {
    struct acpi_sdt_header header;
    uint32_t table_pointers[];
} __attribute__((packed));

/**
 * @brief Find an ACPI table within the RSDT (Root System Description Table).
 * @param rsdt A pointer to the RSDT structure.
 * @param signature The 4-character signature of the table to find.
 * @return A pointer to the found ACPI table, or NULL if not found.
 */
void *acpi_find_table(struct acpi_rsdt *rsdt, const char signature[4]);

#endif // __ACPI_RSDT_H__