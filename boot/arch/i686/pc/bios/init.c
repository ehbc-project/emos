/*
    From now on, What should we do is initialize the devices needed to load the
    kernel then load and jump to it. There's no need to detect and initialize
    all devices, since the kernel will do the rest.
*/

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <macros.h>

#include "mm/mm.h"
#include <device/driver.h>
#include <interface/char.h>
#include <interface/console.h>
#include <interface/framebuffer.h>

#include "asm/bootinfo.h"
#include "asm/bios/disk.h"
#include "asm/bios/video.h"
#include "asm/bios/keyboard.h"
#include "asm/bios/mem.h"

#include "asm/pci/cfgspace.h"
#include "bus/pci/scan.h"
#include "acpi/rsdp.h"
#include "acpi/rsdt.h"
#include "acpi/fadt.h"

#include "bus/bus.h"
#include "bus/usb/host/xhci.h"
#include "bus/usb/host/uhci.h"

#include "core/panic.h"

#include "asm/io.h"
#include "asm/idt.h"
#include "asm/pic.h"

extern void main(void);

extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);

struct pci_device pci_devices[32];


/*
__attribute__((aligned(4096)))
struct uhci_frame_ptr uhci_frame_list[1024];

__attribute__((aligned(4)))
struct uhci_transfer_descriptor uhci_td_list[1024];

uint8_t uhci_packet_buf[1024];

static void uhci_schedule_transfer(int low_speed, int endpoint, int device_addr, int packet_id, void *buf, long len)
{
    packet_id &= 0x0F;
    packet_id |= (~packet_id & 0x0F) << 4;

    uhci_td_list[0] = (struct uhci_transfer_descriptor) {
        .link_ptr_shr4 = 0,
        .db_sel = 0,
        .qh_td_sel = 0,
        .terminate = 1,
        .short_packet_detect = 0,
        .error_limit = 0,
        .low_speed = low_speed,
        .isochronous_select = 0,
        .interrupt_on_complete = 0,
        .status = 0,
        .actual_len = 0,
        .max_len = len,
        .data_toggle = 0,
        .endpoint = endpoint,
        .device_addr = device_addr,
        .packet_id = packet_id,
        .buffer_ptr = (uint32_t)buf,
    };

    uhci_frame_list[0] = (struct uhci_frame_ptr) {
        .ptr_shr4 = (uint32_t)&uhci_td_list[0] >> 4,
        .qh_td_sel = 0,
        .terminate = 0,
    };
}
    */

static void bios_print_str(const char *str)
{
    for (int i = 0; str[i]; i++) {
        _pc_bios_tty_output(str[i]);
    }
}

static int verify_acpi_tables(struct acpi_rsdp *rsdp)
{
    if (!acpi_verify_rsdp(rsdp)) return 1;
    
    struct acpi_rsdt *rsdt = (struct acpi_rsdt *)rsdp->rsdt_address;
    if (!rsdt) {
        return 1;
    }

    for (int i = 0; i < (rsdt->header.length - sizeof(rsdt->header)) / sizeof(uint32_t); i++) {
        if (!acpi_verify_table((void *)rsdt->table_pointers[i])) return 1;
    }
    
    return 0;
}

static struct bus root_bus = {
    .next = NULL,
    .name = "root",
    .dev = NULL,
};

static void init_root_bus(struct acpi_rsdp *rsdp)
{
    int skip_legacy = 0, skip_8042 = 0, skip_rtc = 0;
    
    if (rsdp) {
        struct acpi_rsdt *rsdt = (struct acpi_rsdt *)rsdp->rsdt_address;
        if (!rsdt) {
            panic("Could not find RSDT");
        }

        struct acpi_fadt *fadt = acpi_find_table(rsdt, ACPI_FADT_SIGNATURE);
        if (!fadt) {
            panic("Could not find FADT");
        }

        if (fadt->header.revision >= 3) {
            if (!(fadt->iapc_boot_arch & ACPI_FADT_IAPC_BOOT_ARCH_LEGACY_PRESENT)) {
                skip_legacy = 1;
            }
    
            if (!(fadt->iapc_boot_arch & ACPI_FADT_IAPC_BOOT_ARCH_8042_PRESENT)) {
                skip_8042 = 1;
            }
    
            if (fadt->iapc_boot_arch & ACPI_FADT_IAPC_BOOT_ARCH_CMOSRTC_NPRESENT) {
                skip_rtc = 1;
            }
        }
    }

    register_bus(&root_bus);

    {
        struct device *dev = mm_allocate_clear(1, sizeof(struct device));
        dev->bus = &root_bus;
        struct resource *res = create_resource(NULL);
        res->type = RT_IOPORT;
        res->base = 0x00E9;
        res->limit = 0x00E9;
        res->flags = 0;
        dev->resource = res;
        dev->driver = find_device_driver("debugout");
        register_device(dev);

        freopen_device(stddbg, "dbg0");
    }

    /* find Non-PnP ISA Components */
    if (!skip_8042) {
        struct device *dev = mm_allocate_clear(1, sizeof(struct device));
        dev->bus = &root_bus;
        struct resource *res = create_resource(NULL);
        res->type = RT_IOPORT;
        res->base = 0x0060;
        res->limit = 0x0060;
        res->flags = 0;
        dev->resource = res;
        res = create_resource(res);
        res->type = RT_IOPORT;
        res->base = 0x0064;
        res->limit = 0x0064;
        res->flags = 0;
        res = create_resource(res);
        res->type = RT_IRQ;
        res->base = 0x21;
        res->limit = 0x21;
        res->flags = 0;
        res = create_resource(res);
        res->type = RT_IRQ;
        res->base = 0x2C;
        res->limit = 0x2C;
        res->flags = 0;
        dev->driver = find_device_driver("i8042");
        register_device(dev);
    }

    if (!skip_rtc) {
        struct device *dev = mm_allocate_clear(1, sizeof(struct device));
        dev->bus = &root_bus;
        struct resource *res = create_resource(NULL);
        res->type = RT_IOPORT;
        res->base = 0x0070;
        res->limit = 0x0071;
        res->flags = 0;
        dev->resource = res;
        res = create_resource(res);
        res->type = RT_IRQ;
        res->base = 0x28;
        res->limit = 0x28;
        res->flags = 0;
        dev->driver = find_device_driver("pcrtc");
        register_device(dev);
    }

    if (!skip_legacy) {
        {
            struct device *dev = mm_allocate_clear(1, sizeof(struct device));
            dev->bus = &root_bus;
            struct resource *res = create_resource(NULL);
            res->type = RT_IOPORT;
            res->base = 0x03F8;
            res->limit = 0x03FF;
            res->flags = 0;
            dev->resource = res;
            res = create_resource(res);
            res->type = RT_IRQ;
            res->base = 0x24;
            res->limit = 0x24;
            res->flags = 0;
            dev->driver = find_device_driver("pcuart");
            register_device(dev);
        }
        
        {
            struct device *dev = mm_allocate_clear(1, sizeof(struct device));
            dev->bus = &root_bus;
            struct resource *res = create_resource(NULL);
            res->type = RT_IOPORT;
            res->base = 0x02F8;
            res->limit = 0x02FF;
            res->flags = 0;
            dev->resource = res;
            res = create_resource(res);
            res->type = RT_IRQ;
            res->base = 0x23;
            res->limit = 0x23;
            res->flags = 0;
            dev->driver = find_device_driver("pcuart");
            register_device(dev);
        }

        {
            struct device *dev = mm_allocate_clear(1, sizeof(struct device));
            dev->bus = &root_bus;
            struct resource *res = create_resource(NULL);
            res->type = RT_IOPORT;
            res->base = 0x0278;
            res->limit = 0x027A;
            res->flags = 0;
            dev->resource = res;
            res = create_resource(res);
            res->type = RT_IRQ;
            res->base = 0x27;
            res->limit = 0x27;
            res->flags = 0; 
            dev->driver = find_device_driver("pcieee1284");
            register_device(dev);
        }

        {
            struct device *dev = mm_allocate_clear(1, sizeof(struct device));
            dev->bus = &root_bus;
            struct resource *res = create_resource(NULL);
            res->type = RT_IOPORT;
            res->base = 0x03F0;
            res->limit = 0x03F7;
            res->flags = 0;
            dev->resource = res;
            res = create_resource(res);
            res->type = RT_IRQ;
            res->base = 0x26;
            res->limit = 0x26;
            res->flags = 0;
            res = create_resource(res);
            res->type = RT_DMA;
            res->base = 0;
            res->limit = 0;
            res->flags = 0;
            dev->driver = find_device_driver("fdc_isa");
            register_device(dev);
        }

        {
            struct device *dev = mm_allocate_clear(1, sizeof(struct device));
            dev->bus = &root_bus;
            struct resource *res = create_resource(NULL);
            res->type = RT_IOPORT;
            res->base = 0x01F0;
            res->limit = 0x01F7;
            res->flags = 0;
            dev->resource = res;
            res = create_resource(res);
            res->type = RT_IOPORT;
            res->base = 0x03F6;
            res->limit = 0x03F7;
            res->flags = 0;
            res = create_resource(res);
            res->type = RT_IRQ;
            res->base = 0x2E;
            res->limit = 0x2E;
            res->flags = 0;
            dev->driver = find_device_driver("ide_isa");
            register_device(dev);
        }

        {
            struct device *dev = mm_allocate_clear(1, sizeof(struct device));
            dev->bus = &root_bus;
            struct resource *res = create_resource(NULL);
            res->type = RT_IOPORT;
            res->base = 0x0170;
            res->limit = 0x0177;
            res->flags = 0;
            dev->resource = res;
            res = create_resource(res);
            res->type = RT_IOPORT;
            res->base = 0x0376;
            res->limit = 0x0377;
            res->flags = 0;
            res = create_resource(res);
            res->type = RT_IRQ;
            res->base = 0x2F;
            res->limit = 0x2F;
            res->flags = 0;
            dev->driver = find_device_driver("ide_isa");
            register_device(dev);
        }
    }

    struct vbe_controller_info vbe_info;
    int vbe_found = !_pc_bios_get_vbe_controller_info(&vbe_info);
    
    {
        struct device *dev = mm_allocate_clear(1, sizeof(struct device));
        dev->bus = &root_bus;
        dev->driver = find_device_driver(vbe_found ? "vbe_video" : "vga");
        register_device(dev);
    }
}

static volatile uint64_t global_tick = 0;

uint64_t get_global_tick(void)
{
    return global_tick;
}

static uint8_t rtc_century_offset = 0x00;

static void raw_print(struct console_char_cell *buf, const char *str, int len)
{
    for (int i = 0; i < len && str[i]; i++) {
        buf[i].attr = (struct console_char_attributes){
            .bg_color = 0x000000,
            .fg_color = 0xFFFFFF,
        };
        buf[i].codepoint = str[i];
    }
}

static void rtc_isr(struct device *dev, int num)
{
    static uint8_t prev_sec = 0xFF;
    static uint64_t prev_irq_count = 0;
    uint8_t current_sec, min, hour, day, month, year, century;
    
    _i686_out8(0x70, 0x0C);
    _i686_in8(0x71);

    _i686_out8(0x70, 0x00);
    current_sec = _i686_in8(0x71);
    if (prev_sec != current_sec) {
        _i686_out8(0x70, 0x02);
        min = _i686_in8(0x71);
        _i686_out8(0x70, 0x04);
        hour = _i686_in8(0x71);
        _i686_out8(0x70, 0x07);
        day = _i686_in8(0x71);
        _i686_out8(0x70, 0x08);
        month = _i686_in8(0x71);
        _i686_out8(0x70, 0x09);
        year = _i686_in8(0x71);
        if (rtc_century_offset) {
            _i686_out8(0x70, rtc_century_offset);
            century = _i686_in8(0x71);
        }

        uint64_t irq_count = _pc_get_irq_count();
        prev_sec = current_sec;

        struct device *condev = find_device("console0");
        if (!condev) {
            return;
        }
        const struct console_interface *conif = condev->driver->get_interface(condev, "console");

        struct console_char_cell *buf = conif->get_buffer(condev);
        int con_width;
        conif->get_dimension(condev, &con_width, NULL);
        char str[19];
        snprintf(str, sizeof(str), "%12llu IRQs/s", irq_count - prev_irq_count);
        raw_print(buf + con_width * 2 - 19, str, sizeof(str));
        
        if (rtc_century_offset) {
            snprintf(str, sizeof(str), "%02X%02X-%02X-%02X %02X:%02X:%02X", century, year, month, day, hour, min, current_sec);
        } else {
            snprintf(str, sizeof(str), "  %02X-%02X-%02X %02X:%02X:%02X", year, month, day, hour, min, current_sec);
        }
        raw_print(buf + con_width * 3 - 19, str, sizeof(str));
        conif->invalidate(condev, con_width - 19, 1, con_width - 1, 2);
        conif->flush(condev);
        conif->present(condev);

        prev_irq_count = irq_count;
    }
}

static void pit_isr(struct device *dev, int num)
{
    global_tick++;

    struct device *condev = find_device("console0");
    if (!condev) {
        return;
    }
    const struct console_interface *conif = condev->driver->get_interface(condev, "console");

    struct console_char_cell *buf = conif->get_buffer(condev);
    int con_width;
    conif->get_dimension(condev, &con_width, NULL);
    char str[19];
    snprintf(str, sizeof(str), "tick: %14llu", global_tick);
    raw_print(buf + con_width - 19, str, sizeof(str));
    conif->invalidate(condev, con_width - 19, 0, con_width - 1, 0);
    conif->flush(condev);
    conif->present(condev);
}

__attribute__((noreturn))
void _pc_init(void)
{
    _pc_remap_pic_int(0x20, 0x28);
    _pc_init_idt();

    bios_print_str("Starting bootloader...\r\n");
    for (int i = 0; &(&__init_array_start)[i] != &__init_array_end; i++) {
        (&__init_array_start)[i]();
    }

    _i686_out8(0x70, 0x8A);
    uint8_t prev = _i686_in8(0x71);
    _i686_out8(0x70, 0x8A);
    _i686_out8(0x71, (prev & 0xF0) | 0x0C);

    _i686_out8(0x70, 0x8B);
    prev = _i686_in8(0x71);
    _i686_out8(0x70, 0x8B);
    _i686_out8(0x71, prev | 0x40);

    _pc_set_interrupt_handler(0x20, NULL, pit_isr);
    _pc_set_interrupt_handler(0x28, NULL, rtc_isr);

    static const uint16_t pit_value = 1193182 / 20;
    _i686_out8(0x43, 0x34);
    _i686_out8(0x40, pit_value & 0xFF);
    _i686_out8(0x40, (pit_value >> 8) & 0xFF);
    
    struct acpi_rsdp *rsdp = acpi_find_rsdp();
    if (rsdp) {
        if (verify_acpi_tables(rsdp)) {
            panic("ACPI table checksum failed");
        }
        struct acpi_fadt *fadt = acpi_find_table((struct acpi_rsdt *)rsdp->rsdt_address, ACPI_FADT_SIGNATURE);
        if (fadt) {
            rtc_century_offset = fadt->century;
        }    
    }

    _i686_enable_interrupt();

    init_root_bus(rsdp);

    // struct vbe_controller_info vbe_info;
    // _pc_bios_get_vbe_controller_info(&vbe_info);
    // /* List VBE Controller Info */
    // printf("VBE Controller Info: \r\n");
    // printf("\tSignature: %s\r\n", vbe_info.signature);
    // printf("\tVersion: 0x%04X\r\n", vbe_info.vbe_version);
    // printf("\tOEM String: %s\r\n", (const char *)((vbe_info.oem_string.segment << 4) + vbe_info.oem_string.offset));
    // printf("\tCapabilities: 0x%08lX\r\n", vbe_info.capabilities);
    // printf("\tOEM Vendor Name: %s\r\n", (const char *)((vbe_info.oem_vendor_name_ptr.segment << 4) + vbe_info.oem_vendor_name_ptr.offset));
    // printf("\tOEM Product Name: %s\r\n", (const char *)((vbe_info.oem_product_name_ptr.segment << 4) + vbe_info.oem_product_name_ptr.offset));
    // printf("\tOEM Product Rev: %s\r\n", (const char *)((vbe_info.oem_product_rev_ptr.segment << 4) + vbe_info.oem_product_rev_ptr.offset));
    // printf("\r\n");

    // const struct vbe_pm_interface *pm_interface;
    // _pc_bios_get_vbe_pm_interface(&pm_interface);
    // printf("VBE Protected Mode Interface: ");
    // if (pm_interface) {
    //     /* List VBE Protected Mode Interface */
    //     printf("0x%08lX\r\n", (uint32_t)pm_interface);
    //     printf("\tSet Window: ");
    //     if (pm_interface->set_window) {
    //         printf("0x%08lX\r\n", (uint32_t)pm_interface + pm_interface->set_window);
    //     } else {
    //         printf("Not supported\r\n");
    //     }
    //     printf("\tSet Display Start: ");
    //     if (pm_interface->set_display_start) {
    //         printf("0x%08lX\r\n", (uint32_t)pm_interface + pm_interface->set_display_start);
    //     } else {
    //         printf("Not supported\r\n");
    //     }
    //     printf("\tSet Primary Palette Data: ");
    //     if (pm_interface->set_primary_palette_data) {
    //         printf("0x%08lX\r\n", (uint32_t)pm_interface + pm_interface->set_primary_palette_data);
    //     } else {
    //         printf("Not supported\r\n");
    //     }
    //     printf("\tPort Memory Locations: ");
    //     if (pm_interface->port_mem_locations) {
    //         printf("0x%08lX\r\n", (uint32_t)pm_interface + pm_interface->port_mem_locations);
    //     } else {
    //         printf("Not supported\r\n");
    //     }
    // } else {
    //     printf("Not supported\r\n");
    // }

    // /* List PCI Devices */
    // int pci_count = pci_host_scan(pci_devices, ARRAY_SIZE(pci_devices));

    // /* Find xHCI Controller */
    // int xhci_device_index = -1;
    // for (int i = 0; i < pci_count; i++) {
    //     if (pci_devices[i].base_class == 0x0C &&
    //         pci_devices[i].sub_class == 0x03 && 
    //         pci_devices[i].interface == 0x30) {
    //         printf("xHCI Controller found at 0x%04X, device 0x%04X, function %d\r\n", pci_devices[i].vendor_id, pci_devices[i].device_id, pci_devices[i].function);

    //         xhci_device_index = i;
    //     }
    // }

    // if (xhci_device_index >= 0) {
    //     /* Get MMIO Address of xHCI Controller */
    //     uint32_t xhci_mmio_address = _bus_pci_cfg_read(
    //         pci_devices[xhci_device_index].bus,
    //         pci_devices[xhci_device_index].device,
    //         pci_devices[xhci_device_index].function,
    //         PCI_CFGSPACE_BAR0,
    //         sizeof(xhci_mmio_address)) & 0xFFFFFFF0;
    //     printf("xHCI MMIO Address: 0x%08lX\r\n", xhci_mmio_address);
    // }

    // /* Find EHCI Controller */
    // int ehci_device_index = -1;
    // for (int i = 0; i < pci_count; i++) {
    //     if (pci_devices[i].base_class == 0x0C &&
    //         pci_devices[i].sub_class == 0x03 && 
    //         pci_devices[i].interface == 0x20) {
    //         printf("EHCI Controller found at 0x%04X, device 0x%04X, function %d\r\n", pci_devices[i].vendor_id, pci_devices[i].device_id, pci_devices[i].function);

    //         ehci_device_index = i;
    //     }
    // }

    // if (ehci_device_index >= 0) {
    //     /* Get MMIO Address of EHCI Controller */
    //     uint32_t ehci_mmio_address = _bus_pci_cfg_read(
    //         pci_devices[ehci_device_index].bus,
    //         pci_devices[ehci_device_index].device,
    //         pci_devices[ehci_device_index].function,
    //         PCI_CFGSPACE_BAR0,
    //         sizeof(ehci_mmio_address)) & 0xFFFFFFF0;
    //     printf("EHCI MMIO Address: 0x%08lX\r\n", ehci_mmio_address);
    // }

    // /* Find UHCI Controller */
    // int uhci_device_index = -1;
    // for (int i = 0; i < pci_count; i++) {
    //     if (pci_devices[i].base_class == 0x0C &&
    //         pci_devices[i].sub_class == 0x03 && 
    //         pci_devices[i].interface == 0x00) {
    //         printf("UHCI Controller found at 0x%04X, device 0x%04X, function %d\r\n", pci_devices[i].vendor_id, pci_devices[i].device_id, pci_devices[i].function);

    //         uhci_device_index = i;
    //     }
    // }
    // struct pci_device *uhci_device = &pci_devices[uhci_device_index];

    // uint16_t uhci_pmio_address;
    // if (uhci_device_index >= 0) {
    //     /* Get PMIO Address of UHCI Controller */
    //     uhci_pmio_address = _bus_pci_cfg_read(
    //         pci_devices[uhci_device_index].bus,
    //         pci_devices[uhci_device_index].device,
    //         pci_devices[uhci_device_index].function,
    //         PCI_CFGSPACE_BAR4,
    //         sizeof(uhci_pmio_address)) & 0xFFFFFFFC;
    //     printf("UHCI I/O Port Address: 0x%08X\r\n", uhci_pmio_address);
    // }

    // /* PC Speaker */
    // _i686_out8(0x43, 0xb6);
    // _i686_out8(0x42, (uint8_t)(pit_value / 10));
    // _i686_out8(0x42, (uint8_t)((pit_value / 10) >> 8));

    // for (int i = 0; i < 4; i++) {
    //     uint64_t tick_start = global_tick;
    //     uint8_t tmp = _i686_in8(0x61);
    //     _i686_out8(0x61, tmp | 3);
    //     while (global_tick - tick_start < 50) {}
    //     tick_start = global_tick;
    //     _i686_out8(0x61, tmp & 0xFC);
    //     while (global_tick - tick_start < 50) {}
    // }

    // /* Disable BIOS USB emulation */
    // _bus_pci_cfg_write(0, 0, 0, 0xC0, 0x2000, sizeof(uint16_t));
    // _bus_pci_cfg_write(
    //     uhci_device->bus,
    //     uhci_device->device,
    //     uhci_device->function,
    //     PCI_CFGSPACE_COMMAND,
    //     PCI_COMMAND_BUS_MASTER,
    //     sizeof(uint16_t));

    // /* initialize bus */
    // uint16_t uhci_pcicfgspace_command = _bus_pci_cfg_read(uhci_device->bus, uhci_device->device, uhci_device->function, PCI_CFGSPACE_COMMAND, sizeof(uhci_pcicfgspace_command));
    // _bus_pci_cfg_write(uhci_device->bus, uhci_device->device, uhci_device->function, PCI_CFGSPACE_COMMAND, uhci_pcicfgspace_command | 0x0007, sizeof(uhci_pcicfgspace_command));

    // /* Reset host controller */
    // _i686_out16(uhci_pmio_address + UHCI_IOREG_USBCMD, 0x0006);

    // /* Wait and clear GRESET */
    // for (int i = 0; i < (1 << 20); i++) {}
    // _i686_out16(uhci_pmio_address + UHCI_IOREG_USBCMD, 0x0000);

    // /* Wait for host controller reset */
    // while (_i686_in16(uhci_pmio_address + UHCI_IOREG_USBCMD) & 0x20) {}

    // /* Initialize frame list */
    // for (int i = 0; i < sizeof(uhci_frame_list) / sizeof(uhci_frame_list[0]); i++) {
    //     uhci_frame_list[i] = (struct uhci_frame_ptr){ .ptr_shr4 = 0, .qh_td_sel = 0, .terminate = 1 };
    // }

    // /* Set Values */
    // _i686_out16(uhci_pmio_address + UHCI_IOREG_USBINTR, 0x000F);
    // _i686_out16(uhci_pmio_address + UHCI_IOREG_FRNUM, 0x0000);
    // _i686_out8(uhci_pmio_address + UHCI_IOREG_SOFMOD, 0x40);
    // _i686_out32(uhci_pmio_address + UHCI_IOREG_FRBASE, (uint32_t)uhci_frame_list);

    // /* init status flags */
    // _i686_out16(uhci_pmio_address + UHCI_IOREG_USBSTS, 0xFFFF);

    // /* Start host controller */
    // _i686_out16(uhci_pmio_address + UHCI_IOREG_USBCMD, 0x0081);

    // /* Scan, enable, and reset controller ports */
    // for (int i = 0; i < 2; i++) {
    //     uint16_t portsc =  _i686_in16(uhci_pmio_address + (i ? UHCI_IOREG_PORTSC1 : UHCI_IOREG_PORTSC2));
    //     if (portsc & 0x0001) {
    //         /* reset port */
    //         portsc |= 0x0200;
    //         _i686_out16(uhci_pmio_address + (i ? UHCI_IOREG_PORTSC1 : UHCI_IOREG_PORTSC2), portsc);
    //         for (int i = 0; i < 1048576; i++) {}
    //         portsc &= ~0x0200;
    //         _i686_out16(uhci_pmio_address + (i ? UHCI_IOREG_PORTSC1 : UHCI_IOREG_PORTSC2), portsc);
    //         portsc =  _i686_in16(uhci_pmio_address + (i ? UHCI_IOREG_PORTSC1 : UHCI_IOREG_PORTSC2));

    //         /* enable port */
    //         portsc |= 0x0004;
    //         _i686_out16(uhci_pmio_address + (i ? UHCI_IOREG_PORTSC1 : UHCI_IOREG_PORTSC2), portsc);
    //         portsc =  _i686_in16(uhci_pmio_address + (i ? UHCI_IOREG_PORTSC1 : UHCI_IOREG_PORTSC2));
    //     }

    //     printf("\tPort #%d: connected=%s, enabled=%s, speed=%s\r\n", i, (portsc & 0x0001) ? "true" : "false", (portsc & 0x0004) ? "true" : "false", (portsc & 0x0100) ? "low" : "full");
    // }

    // printf("USBCMD: 0x%04X\r\n", _i686_in16(uhci_pmio_address + UHCI_IOREG_USBCMD));
    // printf("USBSTS: 0x%04X\r\n", _i686_in16(uhci_pmio_address + UHCI_IOREG_USBSTS));
    // printf("USBINTR: 0x%04X\r\n", _i686_in16(uhci_pmio_address + UHCI_IOREG_USBINTR));
    // printf("FRNUM: 0x%04X\r\n", _i686_in16(uhci_pmio_address + UHCI_IOREG_FRNUM));
    // printf("FRBASE: 0x%08lX\r\n", _i686_in32(uhci_pmio_address + UHCI_IOREG_FRBASE));
    // printf("SOFMOD: 0x%02X\r\n", _i686_in8(uhci_pmio_address + UHCI_IOREG_SOFMOD));
    // printf("PORTSC1: 0x%04X\r\n", _i686_in16(uhci_pmio_address + UHCI_IOREG_PORTSC1));
    // printf("PORTSC2: 0x%04X\r\n", _i686_in16(uhci_pmio_address + UHCI_IOREG_PORTSC2));

    main();

    panic("Kernel returned");
}
