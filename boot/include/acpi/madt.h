#ifndef __ACPI_MADT_H__
#define __ACPI_MADT_H__

#include <stdint.h>

#include <acpi/sdt.h>

#define ACPI_MADT_SIGNATURE "APIC"

#define ACPI_MADT_ENTRY_TYPE_LAPIC 0x00
#define ACPI_MADT_ENTRY_TYPE_IOAPIC 0x01
#define ACPI_MADT_ENTRY_TYPE_INT_SRC_OVERRIDE 0x02
#define ACPI_MADT_ENTRY_TYPE_NMI_SRC 0x03
#define ACPI_MADT_ENTRY_TYPE_LAPIC_NMI 0x04
#define ACPI_MADT_ENTRY_TYPE_LAPIC_ADDR_OVERRIDE 0x05

struct acpi_madt {
    struct acpi_sdt_header header;
    uint32_t lapic_address;
    uint32_t flags;
};

struct madt_entry_header {
    uint8_t type;
    uint8_t length;
} __attribute__((packed));

struct madt_entry_lapic {
    struct madt_entry_header header;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed));

struct madt_entry_ioapic {
    struct madt_entry_header header;
    uint8_t io_apic_id;
    uint8_t reserved;
    uint32_t io_apic_address;
    uint32_t global_system_interrupt_base;
} __attribute__((packed));

struct madt_entry_int_src_override {
    struct madt_entry_header header;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} __attribute__((packed));

struct madt_entry_nmi_src {
    struct madt_entry_header header;
    uint8_t nmi_source;
    uint8_t reserved;
    uint16_t flags;
    uint32_t global_system_interrupt;
} __attribute__((packed));

struct madt_entry_lapic_nmi {
    struct madt_entry_header header;
    uint8_t acpi_processor_id;
    uint16_t flags;
    uint8_t lint;
} __attribute__((packed));

struct madt_entry_lapic_addr_override {
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
