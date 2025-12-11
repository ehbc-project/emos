/*
    From now on, What should we do is initialize the devices needed to load the
    kernel then load and jump to it. There's no need to detect and initialize
    all devices, since the kernel will do the rest.
*/

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <eboot/asm/bios/bootinfo.h>
#include <eboot/asm/bios/disk.h>
#include <eboot/asm/bios/video.h>
#include <eboot/asm/bios/keyboard.h>
#include <eboot/asm/bios/mem.h>
#include <eboot/asm/pci/cfgspace.h>
#include <eboot/asm/io.h>
#include <eboot/asm/idt.h>
#include <eboot/asm/isr.h>
#include <eboot/asm/pic.h>

#include <eboot/compiler.h>
#include <eboot/status.h>
#include <eboot/panic.h>
#include <eboot/macros.h>
#include <eboot/debug.h>
#include <eboot/mm.h>
#include <eboot/device.h>
#include <eboot/filesystem.h>
#include <eboot/interface/char.h>
#include <eboot/interface/block.h>
#include <eboot/interface/console.h>
#include <eboot/interface/framebuffer.h>

#include <uacpi/uacpi.h>
#include <uacpi/event.h>
#include <uacpi/tables.h>

extern void main(void);

extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);

static void bios_print_str(const char *str)
{
    for (int i = 0; str[i]; i++) {
        _pc_bios_video_write_tty(str[i]);
    }
}

static ssize_t early_stderr_write(void *cookie, const char *buf, size_t count)
{
    for (size_t i = 0; buf[i] && i < count; i++) {
        if (buf[i] == '\n') {
            _pc_bios_video_write_tty('\r');
        }
        _pc_bios_video_write_tty(buf[i]);
    }

    return count;
}

static const struct cookie_io_functions early_stderr_io = {
    .write = early_stderr_write,
};

static ssize_t early_stddbg_write(void *cookie, const char *buf, size_t count)
{
    for (size_t i = 0; buf[i] && i < count; i++) {
        io_out8(0x00E9, buf[i]);
    }

    return count;
}

static const struct cookie_io_functions early_stddbg_io = {
    .write = early_stddbg_write,
};

static status_t init_nonpnp_devices(int has_acpi)
{
    status_t status;
    int skip_legacy = 0, skip_8042 = 0, skip_rtc = 0;
    struct acpi_fadt *fadt;
    
    if (has_acpi) {
        if (uacpi_table_fadt(&fadt)) {
            panic(STATUS_UNKNOWN_ERROR, "Could not find FADT");
        }

        if (fadt->hdr.revision >= 3) {
            if (!(fadt->iapc_boot_arch & ACPI_IA_PC_LEGACY_DEVS)) {
                skip_legacy = 1;
            }
    
            if (!(fadt->iapc_boot_arch & ACPI_IA_PC_8042)) {
                skip_8042 = 1;
            }
    
            if (fadt->iapc_boot_arch & ACPI_IA_PC_NO_CMOS_RTC) {
                skip_rtc = 1;
            }
        }
    }

#ifndef NDEBUG
    {
        struct device *dev;
        struct device_driver *drv;

        struct resource res[] = {
            {
                .type = RT_IOPORT,
                .base = 0x00E9,
                .limit = 0x00E9,
                .flags = 0,
            },
        };

        status = device_driver_find("debugout", &drv);
        if (!CHECK_SUCCESS(status)) return status;

        status = drv->probe(&dev, drv, NULL, res, ARRAY_SIZE(res));
        if (!CHECK_SUCCESS(status)) return status;

        if (freopendevice("dbg0", stddbg)) return STATUS_UNKNOWN_ERROR;
    }

#endif

    /* find Non-PnP ISA Components */
    if (!skip_8042) {
        struct device *dev;
        struct device_driver *drv;

        struct resource res[] = {
            {
                .type = RT_IOPORT,
                .base = 0x0060,
                .limit = 0x0060,
                .flags = 0,
            },
            {
                .type = RT_IOPORT,
                .base = 0x0064,
                .limit = 0x0064,
                .flags = 0,
            },
            {
                .type = RT_IRQ,
                .base = 0x21,
                .limit = 0x21,
                .flags = 0,
            },
            {
                .type = RT_IRQ,
                .base = 0x2C,
                .limit = 0x2C,
                .flags = 0,
            },
        };

        status = device_driver_find("i8042", &drv);
        if (!CHECK_SUCCESS(status)) return status;

        status = drv->probe(&dev, drv, NULL, res, ARRAY_SIZE(res));
        if (!CHECK_SUCCESS(status)) return status;
    }

    if (!skip_rtc) {
        struct device *dev;
        struct device_driver *drv;

        struct resource res[] = {
            {
                .type = RT_IOPORT,
                .base = 0x0070,
                .limit = 0x0071,
                .flags = 0,
            },
            {
                .type = RT_IRQ,
                .base = 0x28,
                .limit = 0x28,
                .flags = 0,
            },
        };

        status = device_driver_find("rtc_isa", &drv);
        if (!CHECK_SUCCESS(status)) return status;

        status = drv->probe(&dev, drv, NULL, res, ARRAY_SIZE(res));
        if (!CHECK_SUCCESS(status)) return status;
    }

    if (!skip_legacy) {
        if (0) {
            struct device *dev;
            struct device_driver *drv;

            struct resource res[] = {
                {
                    .type = RT_IOPORT,
                    .base = 0x03F8,
                    .limit = 0x03FF,
                    .flags = 0,
                },
                {
                    .type = RT_IRQ,
                    .base = 0x24,
                    .limit = 0x24,
                    .flags = 0,
                },
            };

            status = device_driver_find("uart_isa", &drv);
            if (!CHECK_SUCCESS(status)) return status;
    
            status = drv->probe(&dev, drv, NULL, res, ARRAY_SIZE(res));
            if (!CHECK_SUCCESS(status)) return status;
        }
        
        if (0) {
            struct device *dev;
            struct device_driver *drv;
    
            struct resource res[] = {
                {
                    .type = RT_IOPORT,
                    .base = 0x02F8,
                    .limit = 0x02FF,
                    .flags = 0,
                },
                {
                    .type = RT_IRQ,
                    .base = 0x23,
                    .limit = 0x23,
                    .flags = 0,
                },
            };

            status = device_driver_find("i8042", &drv);
            if (!CHECK_SUCCESS(status)) return status;
    
            status = drv->probe(&dev, drv, NULL, res, ARRAY_SIZE(res));
            if (!CHECK_SUCCESS(status)) return status;
        }

        if (0) {
            struct device *dev;
            struct device_driver *drv;
    
            struct resource res[] = {
                {
                    .type = RT_IOPORT,
                    .base = 0x0278,
                    .limit = 0x027A,
                    .flags = 0,
                },
                {
                    .type = RT_IRQ,
                    .base = 0x27,
                    .limit = 0x27,
                    .flags = 0, 
                },
            };

            status = device_driver_find("ieee1284_isa", &drv);
            if (!CHECK_SUCCESS(status)) return status;
    
            status = drv->probe(&dev, drv, NULL, res, ARRAY_SIZE(res));
            if (!CHECK_SUCCESS(status)) return status;
        }

        {
            struct device *dev;
            struct device_driver *drv;
    
            struct resource res[] = {
                {
                    .type = RT_IOPORT,
                    .base = 0x03F0,
                    .limit = 0x03F7,
                    .flags = 0,
                },
                {
                    .type = RT_IRQ,
                    .base = 0x26,
                    .limit = 0x26,
                    .flags = 0,
                },
                {
                    .type = RT_DMA,
                    .base = 0,
                    .limit = 0,
                    .flags = 0,
                },
            };

            status = device_driver_find("fdc_isa", &drv);
            if (!CHECK_SUCCESS(status)) return status;
    
            status = drv->probe(&dev, drv, NULL, res, ARRAY_SIZE(res));
            if (!CHECK_SUCCESS(status)) return status;
        }

        {
            struct device *dev;
            struct device_driver *drv;
    
            struct resource res[] = {
                {
                    .type = RT_IOPORT,
                    .base = 0x01F0,
                    .limit = 0x01F7,
                    .flags = 0,
                },
                {
                    .type = RT_IOPORT,
                    .base = 0x03F6,
                    .limit = 0x03F7,
                    .flags = 0,
                },
                {
                    .type = RT_IRQ,
                    .base = 0x2E,
                    .limit = 0x2E,
                    .flags = 0,
                },
            };

            status = device_driver_find("ide_isa", &drv);
            if (!CHECK_SUCCESS(status)) return status;
    
            status = drv->probe(&dev, drv, NULL, res, ARRAY_SIZE(res));
            if (!CHECK_SUCCESS(status)) return status;
        }

        {
            struct device *dev;
            struct device_driver *drv;
    
            struct resource res[] = {
                {
                    .type = RT_IOPORT,
                    .base = 0x0170,
                    .limit = 0x0177,
                    .flags = 0,
                },
                {
                    .type = RT_IOPORT,
                    .base = 0x0376,
                    .limit = 0x0377,
                    .flags = 0,
                },
                {
                    .type = RT_IRQ,
                    .base = 0x2F,
                    .limit = 0x2F,
                    .flags = 0,
                },
            };

            status = device_driver_find("ide_isa", &drv);
            if (!CHECK_SUCCESS(status)) return status;
    
            status = drv->probe(&dev, drv, NULL, res, ARRAY_SIZE(res));
            if (!CHECK_SUCCESS(status)) return status;
        }
    }
    
    {
        struct device *dev;
        struct device_driver *drv;

        status = device_driver_find("vbe", &drv);
        if (!CHECK_SUCCESS(status)) return status;

        status = drv->probe(&dev, drv, NULL, NULL, 0);
        if (!CHECK_SUCCESS(status)) {
            status = device_driver_find("vga", &drv);
            if (!CHECK_SUCCESS(status)) return status;
    
            status = drv->probe(&dev, drv, NULL, NULL, 0);
            if (!CHECK_SUCCESS(status)) return status;
        }
    }

    return 0;
}

static uint8_t rtc_century_offset = 0x00;

static status_t acpi_init(void)
{
    uacpi_status uacpi_status;
    static uint8_t acpi_buf[4096];

    uacpi_status = uacpi_setup_early_table_access(acpi_buf, sizeof(acpi_buf));
    if (uacpi_unlikely_error(uacpi_status)) {
        fprintf(stderr, "uacpi_initialize error: %s", uacpi_status_to_string(uacpi_status));
        return 0x80010000 | uacpi_status;
    }

    struct acpi_fadt *fadt;
    if (uacpi_table_fadt(&fadt)) {
        panic(STATUS_UNKNOWN_ERROR, "Could not find FADT");
    }

    rtc_century_offset = fadt->century;

    return STATUS_SUCCESS;
}

status_t mount_boot_disk(void)
{
    status_t status;
    struct device *bootdisk;
    const struct block_interface *blki;
    uint8_t sect0[512];

    bootdisk = device_get_first_dev();

    for (; bootdisk; bootdisk = bootdisk->next) {
        status = bootdisk->driver->get_interface(bootdisk, "block", (const void **)&blki);
        if (!CHECK_SUCCESS(status)) continue;

        status = blki->read(bootdisk, 0, sect0, 1, NULL);
        if (!CHECK_SUCCESS(status)) continue;

        if (memcmp(_pc_boot_sector, sect0, sizeof(sect0)) == 0) break;
    }

    if (!bootdisk) return STATUS_BOOT_DEVICE_INACCESSIBLE;

    status = filesystem_auto_mount(bootdisk, "boot");
    if (!CHECK_SUCCESS(status)) return status;

    return STATUS_SUCCESS;
}

static volatile uint64_t global_tick = 0;

uint64_t get_global_tick(void)
{
    return global_tick;
}

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
    // static uint8_t prev_sec = 0xFF;
    // static uint64_t prev_irq_count = 0;
    // uint8_t current_sec, min, hour, day, month, year, century;
    // 
    // io_out8(0x70, 0x0C);
    // io_in8(0x71);

    // io_out8(0x70, 0x00);
    // current_sec = io_in8(0x71);
    // if (prev_sec != current_sec) {
    //     io_out8(0x70, 0x02);
    //     min = io_in8(0x71);
    //     io_out8(0x70, 0x04);
    //     hour = io_in8(0x71);
    //     io_out8(0x70, 0x07);
    //     day = io_in8(0x71);
    //     io_out8(0x70, 0x08);
    //     month = io_in8(0x71);
    //     io_out8(0x70, 0x09);
    //     year = io_in8(0x71);
    //     if (rtc_century_offset) {
    //         io_out8(0x70, rtc_century_offset);
    //         century = io_in8(0x71);
    //     }

    //     uint64_t irq_count = _pc_get_irq_count();
    //     prev_sec = current_sec;

    //     struct device *condev = find_device("console0");
    //     if (!condev) {
    //         return;
    //     }
    //     const struct console_interface *conif = condev->driver->get_interface(condev, "console");

    //     struct console_char_cell *buf = conif->get_buffer(condev);
    //     int con_width;
    //     conif->get_dimension(condev, &con_width, NULL);
    //     char str[19];
    //     snprintf(str, sizeof(str), "%12llu IRQs/s", irq_count - prev_irq_count);
    //     raw_print(buf + con_width * 2 - 19, str, sizeof(str));
    //     
    //     if (rtc_century_offset) {
    //         snprintf(str, sizeof(str), "%02X%02X-%02X-%02X %02X:%02X:%02X", century, year, month, day, hour, min, current_sec);
    //     } else {
    //         snprintf(str, sizeof(str), "  %02X-%02X-%02X %02X:%02X:%02X", year, month, day, hour, min, current_sec);
    //     }
    //     raw_print(buf + con_width * 3 - 19, str, sizeof(str));
    //     conif->invalidate(condev, con_width - 19, 1, con_width - 1, 2);
    //     conif->flush(condev);
    //     conif->present(condev);

    //     prev_irq_count = irq_count;
    // }
}

static void pit_isr(struct device *dev, int num)
{
    global_tick++;

    // struct device *condev = find_device("console0");
    // if (!condev) return;
    // const struct console_interface *conif = condev->driver->get_interface(condev, "console");

    // struct console_char_cell *buf = conif->get_buffer(condev);
    // int con_width;
    // ret = conif->get_dimension(condev, &con_width, NULL);
    // if (ret) return;
    // char str[19];
    // snprintf(str, sizeof(str), "tick: %14llu", global_tick);
    // raw_print(buf + con_width - 19, str, sizeof(str));
    // conif->invalidate(condev, con_width - 19, 0, con_width - 1, 0);
    // conif->flush(condev);
    // conif->present(condev);
}

static void bkpt_handler(struct interrupt_frame *frame, struct trap_regs *regs, int num, int error)
{
    fprintf(stderr, "Breakpoint at %04X:%08lX\n", frame->cs, frame->eip);

    fprintf(stderr, "EAX=%08lX EBX=%08lX ECX=%08lX EDX=%08lX\n", regs->eax, regs->ebx, regs->ecx, regs->edx);
    fprintf(stderr, "ESI=%08lX EDI=%08lX EBP=%08lX ESP=%08lX\n", regs->esi, regs->edi, regs->ebp, regs->esp);
    fprintf(stderr, "CS=%04X DS=%04X ES=%04X FS=%04X GS=%04X SS=%04X\n", frame->cs, regs->ds, regs->es, regs->fs, regs->gs, regs->ss);
    fprintf(stderr, "EFLAGS=%08lX\n", frame->eflags);

    uint32_t bp = regs->ebp;
    for (int i = 0; bp; i++) {
        fprintf(stderr, "Frame #%d: %08lX %04X:%08lX\n", i, bp, frame->cs, ((uint32_t *)bp)[1]);
        bp = ((uint32_t *)bp)[0];
    }
}

__noreturn
void _pc_init(void)
{
    status_t status;
    int has_acpi;
    struct chs bootdisk_geom;

    bios_print_str("Starting bootloader...\r\n");

    freopencookie(NULL, "w", early_stderr_io, stderr);
    freopencookie(NULL, "w", early_stddbg_io, stddbg);

    _pc_bios_disk_get_params(_pc_boot_drive, NULL, NULL, &bootdisk_geom, NULL);
    struct chs chs = disk_lba_to_chs(_pc_boot_part_base, bootdisk_geom);
    status = _pc_bios_disk_read(_pc_boot_drive, chs, 1, _pc_boot_sector, NULL);
    fprintf(stderr, "%d %d %d %08X\n", chs.cylinder, chs.head, chs.sector, status);

    _pc_remap_pic_int(0x20, 0x28);
    _pc_init_idt();

    hexdump(stderr, _pc_boot_sector, 16, 0);

    mm_init();

    io_out8(0x70, 0x8A);
    uint8_t prev = io_in8(0x71);
    io_out8(0x70, 0x8A);
    io_out8(0x71, (prev & 0xF0) | 0x0C);

    io_out8(0x70, 0x8B);
    prev = io_in8(0x71);
    io_out8(0x70, 0x8B);
    io_out8(0x71, prev | 0x40);

    _pc_isr_set_trap_handler(0x03, bkpt_handler);
    _pc_isr_set_interrupt_handler(0x20, NULL, pit_isr);
    _pc_isr_set_interrupt_handler(0x28, NULL, rtc_isr);

    for (int i = 0; &(&__init_array_start)[i] != &__init_array_end; i++) {
        (&__init_array_start)[i]();
    }

    static const uint16_t pit_value = 1193182 / 20;
    io_out8(0x43, 0x34);
    io_out8(0x40, pit_value & 0xFF);
    io_out8(0x40, (pit_value >> 8) & 0xFF);
    
    status = acpi_init();
    has_acpi = !CHECK_SUCCESS(status);

    interrupt_enable();

    status = init_nonpnp_devices(has_acpi);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "init_nonpnp_devices() failed: 0x%08X\n", status);
        panic(status, "failed to initialize essential non-PnP devices");
    }

    // /* Disable BIOS USB emulation */
    // _bus_pci_cfg_write16(0, 0, 0, 0xC0, 0x2000);
    // _bus_pci_cfg_write16(
    //     uhci_device->bus,
    //     uhci_device->device,
    //     uhci_device->function,
    //     PCI_CFGSPACE_COMMAND,
    //     PCI_COMMAND_BUS_MASTER,
    // );

    // /* List PCI Devices */
    // int pci_count = pci_host_scan(pci_devices, ARRAY_SIZE(pci_devices));

    // /* PC Speaker */
    // io_out8(0x43, 0xb6);
    // io_out8(0x42, (uint8_t)(pit_value / 10));
    // io_out8(0x42, (uint8_t)((pit_value / 10) >> 8));

    // for (int i = 0; i < 4; i++) {
    //     uint64_t tick_start = global_tick;
    //     uint8_t tmp = io_in8(0x61);
    //     io_out8(0x61, tmp | 3);
    //     while (global_tick - tick_start < 50) {}
    //     tick_start = global_tick;
    //     io_out8(0x61, tmp & 0xFC);
    //     while (global_tick - tick_start < 50) {}
    // }

    status = mount_boot_disk();
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to mount bootloader partition");
    }

    for (;;) {}

    main();

    panic(STATUS_UNKNOWN_ERROR, "Kernel returned");
}

__destructor
static void _pc_fini(void)
{
    _pc_remap_pic_int(0x08, 0x70);
}
