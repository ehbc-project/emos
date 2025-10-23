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

#include <compiler.h>
#include <mm/mm.h>
#include <device/driver.h>
#include <interface/char.h>
#include <interface/console.h>
#include <interface/framebuffer.h>

#include <asm/bootinfo.h>
#include <asm/bios/disk.h>
#include <asm/bios/video.h>
#include <asm/bios/keyboard.h>
#include <asm/bios/mem.h>
#include <asm/pci/cfgspace.h>
#include <sys/io.h>
#include <asm/idt.h>
#include <asm/pic.h>
#include <asm/breakpoint.h>

#include <uacpi/uacpi.h>
#include <uacpi/event.h>
#include <uacpi/tables.h>

#include <bus/bus.h>
#include <bus/pci/scan.h>

#include <debug.h>


extern void main(void);

extern void (*__init_array_start)(void);
extern void (*__init_array_end)(void);

struct pci_device pci_devices[32];

static void bios_print_str(const char *str)
{
    for (int i = 0; str[i]; i++) {
        _pc_bios_tty_output(str[i]);
    }
}

static struct bus root_bus = {
    .next = NULL,
    .name = "root",
    .dev = NULL,
};

static int init_root_bus(int has_acpi)
{
    int ret;
    int skip_legacy = 0, skip_8042 = 0, skip_rtc = 0;
    
    if (has_acpi) {
        struct acpi_fadt *fadt;
        if (uacpi_table_fadt(&fadt)) {
            panic("Could not find FADT");
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

    ret = register_bus(&root_bus);
    if (ret) return ret;

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
        ret = register_device(dev);
        if (ret) return ret;

        ret = freopendevice("dbg0", stddbg);
        if (ret) return ret;
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
        ret = register_device(dev);
        if (ret) return ret;
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
        dev->driver = find_device_driver("rtc_isa");
        ret = register_device(dev);
        if (ret) return ret;
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
            dev->driver = find_device_driver("uart_isa");
            ret = register_device(dev);
            // if (ret) return ret;
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
            dev->driver = find_device_driver("uart_isa");
            ret = register_device(dev);
            // if (ret) return ret;
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
            dev->driver = find_device_driver("ieee1284_isa");
            ret = register_device(dev);
            // if (ret) return ret;
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
            ret = register_device(dev);
            // if (ret) return ret;
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
            ret = register_device(dev);
            // if (ret) return ret;
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
            ret = register_device(dev);
            if (ret) return ret;
        }
    }

    struct vbe_controller_info vbe_info;
    int vbe_found = !_pc_bios_get_vbe_controller_info(&vbe_info);
    
    {
        struct device *dev = mm_allocate_clear(1, sizeof(struct device));
        dev->bus = &root_bus;
        dev->driver = find_device_driver(vbe_found ? "vbe_video" : "vga");
        ret = register_device(dev);
        if (ret) return ret;
    }

    return 0;
}

static uint8_t rtc_century_offset = 0x00;

static int acpi_init(void)
{
    static uint8_t acpi_buf[4096];

    uacpi_status ret = uacpi_setup_early_table_access(acpi_buf, sizeof(acpi_buf));
    if (uacpi_unlikely_error(ret)) {
        fprintf(stderr, "uacpi_initialize error: %s", uacpi_status_to_string(ret));
        return ret;
    }

    struct acpi_fadt *fadt;
    if (uacpi_table_fadt(&fadt)) {
        panic("Could not find FADT");
    }

    rtc_century_offset = fadt->century;

    return 0;
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
    static uint8_t prev_sec = 0xFF;
    static uint64_t prev_irq_count = 0;
    uint8_t current_sec, min, hour, day, month, year, century;
    
    io_out8(0x70, 0x0C);
    io_in8(0x71);

    io_out8(0x70, 0x00);
    current_sec = io_in8(0x71);
    if (prev_sec != current_sec) {
        io_out8(0x70, 0x02);
        min = io_in8(0x71);
        io_out8(0x70, 0x04);
        hour = io_in8(0x71);
        io_out8(0x70, 0x07);
        day = io_in8(0x71);
        io_out8(0x70, 0x08);
        month = io_in8(0x71);
        io_out8(0x70, 0x09);
        year = io_in8(0x71);
        if (rtc_century_offset) {
            io_out8(0x70, rtc_century_offset);
            century = io_in8(0x71);
        }

        uint64_t irq_count = _pc_get_irq_count();
        prev_sec = current_sec;

        // struct device *condev = find_device("console0");
        // if (!condev) {
        //     return;
        // }
        // const struct console_interface *conif = condev->driver->get_interface(condev, "console");

        // struct console_char_cell *buf = conif->get_buffer(condev);
        // int con_width;
        // conif->get_dimension(condev, &con_width, NULL);
        // char str[19];
        // snprintf(str, sizeof(str), "%12llu IRQs/s", irq_count - prev_irq_count);
        // raw_print(buf + con_width * 2 - 19, str, sizeof(str));
        // 
        // if (rtc_century_offset) {
        //     snprintf(str, sizeof(str), "%02X%02X-%02X-%02X %02X:%02X:%02X", century, year, month, day, hour, min, current_sec);
        // } else {
        //     snprintf(str, sizeof(str), "  %02X-%02X-%02X %02X:%02X:%02X", year, month, day, hour, min, current_sec);
        // }
        // raw_print(buf + con_width * 3 - 19, str, sizeof(str));
        // conif->invalidate(condev, con_width - 19, 1, con_width - 1, 2);
        // conif->flush(condev);
        // conif->present(condev);

        prev_irq_count = irq_count;
    }
}

static void pit_isr(struct device *dev, int num)
{
    int ret;

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
    int ret;

    _pc_remap_pic_int(0x20, 0x28);
    _pc_init_idt();

    bios_print_str("Starting bootloader...\r\n");
    for (int i = 0; &(&__init_array_start)[i] != &__init_array_end; i++) {
        (&__init_array_start)[i]();
    }

    io_out8(0x70, 0x8A);
    uint8_t prev = io_in8(0x71);
    io_out8(0x70, 0x8A);
    io_out8(0x71, (prev & 0xF0) | 0x0C);

    io_out8(0x70, 0x8B);
    prev = io_in8(0x71);
    io_out8(0x70, 0x8B);
    io_out8(0x71, prev | 0x40);

    _pc_set_trap_handler(0x03, bkpt_handler);
    _pc_set_interrupt_handler(0x20, NULL, pit_isr);
    _pc_set_interrupt_handler(0x28, NULL, rtc_isr);

    static const uint16_t pit_value = 1193182 / 20;
    io_out8(0x43, 0x34);
    io_out8(0x40, pit_value & 0xFF);
    io_out8(0x40, (pit_value >> 8) & 0xFF);
    
    int acpi_error = acpi_init();

    interrupt_enable();

    ret = init_root_bus(!acpi_error);
    if (ret) {
        panic("Failed to initialize root bus");
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

    main();

    panic("Kernel returned");
}

__destructor
static void _pc_fini(void)
{
    _pc_remap_pic_int(0x08, 0x70);
}
