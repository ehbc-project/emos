#ifndef __ACPI_XSDT_H__
#define __ACPI_XSDT_H__

#include <stdint.h>

#include <acpi/sdt.h>

#define ACPI_XSDT_SIGNATURE "XSDT"

struct acpi_xsdt {
    struct acpi_sdt_header header;
    uint64_t table_pointers[];
} __attribute__((packed));

#endif // __ACPI_XSDT_H__