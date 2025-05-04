#ifndef __ACPI_MADT_H__
#define __ACPI_MADT_H__

#include <stdint.h>

#include "acpi/sdt.h"

#define ACPI_MADT_SIGNATURE "APIC"

struct acpi_madt {
    struct acpi_sdt_header header;
    uint32_t local_apic_address;
    uint32_t flags;
};

struct madt_entry_header {
    uint8_t type;
    uint8_t length;
} __attribute__((packed));

struct madt_entry_local_apic {
    struct madt_entry_header header;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed));

struct madt_entry_io_apic {
    struct madt_entry_header header;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_address;
    uint32_t global_system_interrupt_base;
} __attribute__((packed));

struct madt_entry_int_source_override {
    struct madt_entry_header header;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} __attribute__((packed));

struct madt_entry_nmi_source {
    struct madt_entry_header header;
    uint8_t nmi_source;
    uint8_t reserved;
    uint16_t flags;
    uint32_t global_system_interrupt;
} __attribute__((packed));

struct madt_entry_nmi {
    struct madt_entry_header header;
    uint8_t acpi_processor_id;
    uint16_t flags;
    uint8_t lint;
} __attribute__((packed));

struct madt_entry_local_apic_addr_override {
    struct madt_entry_header header;
    uint16_t reserved;
    uint64_t address;
} __attribute__((packed));

struct madt_entry_local_x2apic {
    struct madt_entry_header header;
    uint16_t reserved;
    uint32_t x2apic_id;
    uint32_t flags;
    uint32_t acpi_id;
} __attribute__((packed));

#endif
