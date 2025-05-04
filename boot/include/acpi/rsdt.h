#ifndef __ACPI_RSDT_H__
#define __ACPI_RSDT_H__

#include <stdint.h>

#include "acpi/sdt.h"

#define ACPI_RSDT_SIGNATURE "RSDT"

struct acpi_rsdt {
    struct acpi_sdt_header header;
    uint32_t table_pointers[];
} __attribute__((packed));

#endif // __ACPI_RSDT_H__