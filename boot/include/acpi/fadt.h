#ifndef __ACPI_XSDT_H__
#define __ACPI_XSDT_H__

#include <stdint.h>

#include <acpi/sdt.h>

#define ACPI_FADT_SIGNATURE "FACP"

#define ACPI_FADT_IAPC_BOOT_ARCH_LEGACY_PRESENT     0x0001
#define ACPI_FADT_IAPC_BOOT_ARCH_8042_PRESENT       0x0002
#define ACPI_FADT_IAPC_BOOT_ARCH_VGA_NPRESENT       0x0004
#define ACPI_FADT_IAPC_BOOT_ARCH_MSI_NSUPPORTED     0x0008
#define ACPI_FADT_IAPC_BOOT_ARCH_PCIE_ASPM_CTRL     0x0010
#define ACPI_FADT_IAPC_BOOT_ARCH_CMOSRTC_NPRESENT   0x0020

struct generic_address {
    uint8_t address_space;
    uint8_t bit_width;
    uint8_t bit_offset;
    uint8_t access_size;
    uint64_t address;
} __attribute__((packed));

struct acpi_fadt {
    struct acpi_sdt_header header;
    uint32_t firmware_ctrl;
    uint32_t dsdt_address;

    /* ACPI 1.0 */
    uint8_t reserved1;
    uint8_t preferred_pm_profile;
    uint16_t sci_interrupt;
    uint32_t smi_command_port;
    uint8_t acpi_enable;
    uint8_t acpi_disable;
    uint8_t s4_bios_req;
    uint8_t pstate_control;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t pm1_event_length;
    uint8_t pm1_control_length;
    uint8_t pm2_control_length;
    uint8_t pm_timer_length;
    uint8_t gpe0_length;
    uint8_t gpe1_length;
    uint8_t gpe1_base;
    uint8_t cstate_control;
    uint16_t worst_c2_latency;
    uint16_t worst_c3_latency;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alarm;
    uint8_t month_alarm;
    uint8_t century;

    /* ACPI 2.0 */
    uint16_t iapc_boot_arch;
    uint8_t reserved2;
    uint32_t flags;
    struct generic_address reset_register;
    uint8_t reset_value;
    uint16_t arm_boot_arch;
    uint8_t fadt_minor_version;
    uint64_t x_firmware_control;
    uint64_t x_dsdt_address;
    struct generic_address x_pm1a_event_block;
    struct generic_address x_pm1b_event_block;
    struct generic_address x_pm1a_control_block;
    struct generic_address x_pm1b_control_block;
    struct generic_address x_pm2_control_block;
    struct generic_address x_pm_timer_block;
    struct generic_address x_gpe0_block;
    struct generic_address x_gpe1_block;
} __attribute__((packed));

#endif // __ACPI_XSDT_H__