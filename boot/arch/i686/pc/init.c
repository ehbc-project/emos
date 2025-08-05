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

#include "bus/usb/host/xhci.h"
#include "bus/usb/host/uhci.h"

#include "core/panic.h"

#include "asm/io.h"

extern void main(void);

extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);

struct pci_device pci_devices[32];

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

static void bios_print_str(const char *str)
{
    for (int i = 0; str[i]; i++) {
        _pc_bios_tty_output(str[i]);
    }
}

__attribute__((noreturn))
void _pc_init(void)
{
    bios_print_str("\r\n");
    bios_print_str("Starting bootloader...\r\n");
    for (int i = 0; &(&__init_array_start)[i] != &__init_array_end; i++) {
        (&__init_array_start)[i]();
    }

    bios_print_str("Finding drivers...\r\n");
    struct device_driver *vbe_driver = find_driver("vbe_video");
    struct device_driver *vconsole_driver = find_driver("vconsole");
    struct device_driver *ansiterm_driver = find_driver("ansiterm");
    struct device_driver *kbd_driver = find_driver("bios_keyboard");
    struct device_driver *bios_tty_driver = find_driver("bios_tty");

    bios_print_str("Probing devices...\r\n");
    struct device *vbe_device = vbe_driver->probe(NULL);
    struct device *vconsole_device = vconsole_driver->probe(vbe_device->id);
    struct device *ansiterm_device = ansiterm_driver->probe(vconsole_device->id);
    struct device *kbd_device = kbd_driver->probe(NULL);
    struct device *bios_tty_device = bios_tty_driver->probe(NULL);

    const struct framebuffer_interface *framebuffer = vbe_driver->get_interface(vbe_device, "framebuffer");
    const struct console_interface *console = vconsole_driver->get_interface(vconsole_device, "console");
    const struct char_interface *bios_tty = bios_tty_driver->get_interface(bios_tty_device, "char");
    const struct char_interface *tty = NULL;
    if (vbe_device && ansiterm_device) {
        tty = ansiterm_driver->get_interface(ansiterm_device, "char");
    }

    freopen_device(stdout, "tty0");
    freopen_device(stderr, "tty0");
    freopen_device(stdin, "kbd");

    if (vbe_device && ansiterm_device) {
        printf("Setting up video...\r\n");
        framebuffer->set_mode(vbe_device, 640, 480, 24);
        console->set_dimension(vconsole_device, 80, 30);

        freopen_device(stdout, "tty");
        freopen_device(stderr, "tty");
    }


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

    // /* List Memory Map */
    // uint32_t cursor = 0;
    // struct smap_entry entry;

    // printf("Memory Map: \r\n");
    // do {
    //     _pc_bios_query_address_map(&cursor, &entry, sizeof(entry));
    //     printf("\t0x%08lX 0x%08lX 0x%08lX\r\n", entry.base_addr_low, entry.length_low, entry.type);
    // } while (cursor);

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
    // if (0) {
    //     const uint16_t div = 1193180 / 1000;
    //     _i686_out8(0x43, 0xb6);
    //     _i686_out8(0x42, (uint8_t)div);
    //     _i686_out8(0x42, (uint8_t)(div >> 8));

    //     uint8_t tmp = _i686_in8(0x61);
    //     if (tmp != (tmp | 3)) {
    //         _i686_out8(0x61, tmp | 3);
    //     }
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
