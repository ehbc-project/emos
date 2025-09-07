#include <shell.h>

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <macros.h>
#include <stdlib.h>
#include <cpuid.h>
#include <ctype.h>
#include <getopt.h>
#include <path.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <interface/char.h>
#include <interface/console.h>
#include <interface/framebuffer.h>
#include <interface/block.h>
#include <interface/hid.h>
#include <bus/pci/scan.h>
#include <acpi/rsdp.h>
#include <acpi/rsdt.h>
#include <acpi/fadt.h>
#include <acpi/madt.h>
#include <asm/bios/video.h>
#include <asm/bios/disk.h>
#include <asm/bios/misc.h>
#include <asm/reboot.h>
#include <asm/poweroff.h>
#include <asm/pci/cfgspace.h>
#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/breakpoint.h>
#include <asm/interrupt.h>
#include <asm/pic.h>
#include <asm/time.h>
#include <asm/boot.h>
#include <core/panic.h>
#include <fs/driver.h>
#include <font.h>

static int readline(char *buf, int len)
{
    int cur = 0;
    char ch;
    do {
        fread(&ch, 1, 1, stdin);
        switch (ch) {
            case '\r':
            case '\n':
                putchar('\n');
                break;
            case '\b':
                if (cur < 1) {
                    break;
                }
                cur--;
                puts("\b \b");
                break;
            default:
                if (cur >= len - 1) {
                    break;
                }
                putchar(ch);
                buf[cur++] = ch;
                break;
        }
    } while (ch != '\r' && ch != '\n');

    buf[cur] = 0;
    return cur;
}

struct shell_instance {
    struct filesystem *fs;
    struct fs_directory *working_dir;
    char working_dir_path[256];
};

struct command {
    const char *name;
    int (*handler)(struct shell_instance *inst, int argc, char **argv);
    const char *help_message;
};

static int tick_handler(struct shell_instance *inst, int argc, char **argv);
static int echo_handler(struct shell_instance *inst, int argc, char **argv);
static int vmode_handler(struct shell_instance *inst, int argc, char **argv);
static int cpuid_handler(struct shell_instance *inst, int argc, char **argv);
static int lspci_handler(struct shell_instance *inst, int argc, char **argv);
static int lsdev_handler(struct shell_instance *inst, int argc, char **argv);
static int acpi_handler(struct shell_instance *inst, int argc, char **argv);
static int testtty_handler(struct shell_instance *inst, int argc, char **argv);
static int testmouse_handler(struct shell_instance *inst, int argc, char **argv);
static int read_handler(struct shell_instance *inst, int argc, char **argv);
static int write_handler(struct shell_instance *inst, int argc, char **argv);
static int in_handler(struct shell_instance *inst, int argc, char **argv);
static int out_handler(struct shell_instance *inst, int argc, char **argv);
static int readblk_handler(struct shell_instance *inst, int argc, char **argv);
static int mount_handler(struct shell_instance *inst, int argc, char **argv);
static int chfs_handler(struct shell_instance *inst, int argc, char **argv);
static int lsfs_handler(struct shell_instance *inst, int argc, char **argv);
static int lsfile_handler(struct shell_instance *inst, int argc, char **argv);
static int chdir_handler(struct shell_instance *inst, int argc, char **argv);
static int readfile_handler(struct shell_instance *inst, int argc, char **argv);
static int dispimg_handler(struct shell_instance *inst, int argc, char **argv);
static int lsint_handler(struct shell_instance *inst, int argc, char **argv);
static int chainload_handler(struct shell_instance *inst, int argc, char **argv);
static int bootnext_handler(struct shell_instance *inst, int argc, char **argv);
static int drvinfo_handler(struct shell_instance *inst, int argc, char **argv);
static int usefont_handler(struct shell_instance *inst, int argc, char **argv);
static int jump_handler(struct shell_instance *inst, int argc, char **argv);
static int help_handler(struct shell_instance *inst, int argc, char **argv);
static int reboot_handler(struct shell_instance *inst, int argc, char **argv);
static int poweroff_handler(struct shell_instance *inst, int argc, char **argv);
static int exit_handler(struct shell_instance *inst, int argc, char **argv);
static int shell_handler(struct shell_instance *inst, int argc, char **argv);

static const struct command commands[] = {
    { "acpi", acpi_handler, "Show ACPI informations" },
    { "bootnext", bootnext_handler, "Boot from next device" },
    { "cd", chdir_handler, "Change working directory" },
    { "cf", chfs_handler, "Change filesystem" },
    { "chainload", chainload_handler, "Boot from other disk" },
    { "chdir", chdir_handler, "Change working directory" },
    { "chfs", chfs_handler, "Change filesystem" },
    { "cpuid", cpuid_handler, "Invoke CPUID" },
    { "drvinfo", drvinfo_handler, "Show drive information" },
    { "dispimg", dispimg_handler, "Display a .bmp image file" },
    { "echo", echo_handler, "Write arguments to the stanard output" },
    { "exit", exit_handler, "Exit from shell" },
    { "help", help_handler, "Print this message" },
    { "in", in_handler, "Read input from I/O port" },
    { "jump", jump_handler, "Jump to address" },
    { "lsdev", lsdev_handler, "List devices" },
    { "lsfs", lsfs_handler, "List filesystems" },
    { "lsfile", lsfile_handler, "List files of current directory" },
    { "lsint", lsint_handler, "List interrupt status" },
    { "lspci", lspci_handler, "List PCI devices" },
    { "mount", mount_handler, "Mount filesystem" },
    { "out", out_handler, "Write output to I/O port" },
    { "poweroff", poweroff_handler, "Power off the computer" },
    { "read", read_handler, "Read value from memory" },
    { "readblk", readblk_handler, "Read block from block device" },
    { "readfile", readfile_handler, "Read entire file to stdio" },
    { "reboot", reboot_handler, "Reboot the computer" },
    { "shell", shell_handler, "Create a new instance of shell" },
    { "testmouse", testmouse_handler, "Test mouse functions" },
    { "testtty", testtty_handler, "Test TTY functions" },
    { "tick", tick_handler, "Show current tick value" },
    { "usefont", usefont_handler, "Change terminal font" },
    { "vmode", vmode_handler, "List or set video mode" },
    { "write", write_handler, "Write value to memory" },
};

static int tick_handler(struct shell_instance *inst, int argc, char **argv)
{
    printf("%llu\n", get_global_tick());

    return 0;
}

static int echo_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (argc < 2) return 0;

    char *message = argv[1];
    printf("%s\n", argv[1]);

    return 0;
}

static int vmode_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s [-l] [mode]\n", argv[0]);
        return 1;
    }

    const struct option opts[] = {
        { "list", optional_argument, 0, 'l' },
        { NULL, 0, NULL, 0 },
    };

    int opt, err;
    getopt_init();
    while ((opt = getopt_long(argc, argv, "l", opts, NULL)) != -1) {
        switch (opt) {
            case 'l': {
                static const char *memory_models[] = {
                    "Text",
                    "CGA",
                    "Hercules",
                    "Planar",
                    "Packed",
                    "Non-chain",
                    "Direct",
                    "YUV",
                };

                struct vbe_controller_info vbe_info;
                err = _pc_bios_get_vbe_controller_info(&vbe_info);
                if (err) {
                    fprintf(stderr, "%s: could not list video modes\n", argv[0]);
                    return 1;
                }
            
                uint16_t *mode_list = (uint16_t *)((vbe_info.video_modes.segment << 4) + vbe_info.video_modes.offset);
                struct vbe_video_mode_info vbe_mode_info;
                uint16_t mode = 0xFFFF;
                printf("Video Modes:\r\n");
                for (int i = 0; mode_list[i] != 0xFFFF; i++) {
                    err = _pc_bios_get_vbe_video_mode_info(mode_list[i], &vbe_mode_info);
                    if (err) {
                        fprintf(stderr, "%s: could not get video mode info\n", argv[0]);
                        return 1;
                    }
                    printf("Mode %03X: w=%4d h=%4d bpp=%2d addr=%08lX attr=%04X mm=%s\r\n", mode_list[i], vbe_mode_info.width, vbe_mode_info.height, vbe_mode_info.bpp, vbe_mode_info.framebuffer, vbe_mode_info.attributes, memory_models[vbe_mode_info.memory_model]);
                }

                return 0;
            }
            default:
                printf("usage: %s [-l] [mode]\n", argv[0]);
                return 1;
        }
    }

    int mode = strtol(argv[1], NULL, 16);
    struct vbe_video_mode_info vbe_mode_info;
    err = _pc_bios_get_vbe_video_mode_info(mode, &vbe_mode_info);
    if (err) {
        fprintf(stderr, "%s: could not get video mode info\n", argv[0]);
        return 1;
    }

    struct device *fbdev = find_device("video0");
    struct device *condev = find_device("console0");
    const struct console_interface *conif = condev->driver->get_interface(condev, "console");
    
    if (vbe_mode_info.memory_model == VBEMM_TEXT) {
        const struct console_interface *fbconif = fbdev->driver->get_interface(fbdev, "console");
        err = fbconif->set_dimension(fbdev, vbe_mode_info.width, vbe_mode_info.height);
        if (err) {
            fprintf(stderr, "%s: could not switch video mode\n", argv[0]);
            return 1;
        }
        err = conif->set_dimension(condev, -1, -1);
        if (err) {
            fprintf(stderr, "%s: unknown error\n", argv[0]);
            return 1;
        }
    } else {
        const struct framebuffer_interface *framebuffer = fbdev->driver->get_interface(fbdev, "framebuffer");
        err = framebuffer->set_mode(fbdev, vbe_mode_info.width, vbe_mode_info.height, vbe_mode_info.bpp);
        if (err) {
            fprintf(stderr, "%s: could not switch video mode\n", argv[0]);
            return 1;
        }
        err = conif->set_dimension(condev, vbe_mode_info.width / 8, vbe_mode_info.height / 16);
        if (err) {
            fprintf(stderr, "%s: could not set console dimension\n", argv[0]);
            return 1;
        }
        printf("Framebuffer: 0x%p\n", framebuffer->get_framebuffer(fbdev));
    }
    printf("Text buffer: 0x%p\n", (void *)conif->get_buffer(condev));

    return 0;
}

static int cpuid_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s num\n", argv[0]);
        return 1;
    }

    int request = atoi(argv[1]);

    uint32_t eax, ebx, ecx, edx;
    __cpuid(request, eax, ebx, ecx, edx);

    if (request == 0) {
        uint32_t vendor_str[3] = { ebx, edx, ecx };
        printf("eax=%08lX\n", eax);
        printf("ebx_edx_ecx=%12s\n", (char *)vendor_str);
    } else {
        printf("eax=%08lX\n", eax);
        printf("ebx=%08lX\n", ebx);
        printf("edx=%08lX\n", edx);
        printf("ecx=%08lX\n", ecx);
    }

    return 0;
}

static int lspci_handler(struct shell_instance *inst, int argc, char **argv)
{
    struct pci_device pci_devices[32];
    int pci_count = pci_host_scan(pci_devices, ARRAY_SIZE(pci_devices));

    for (int i = 0; i < pci_count; i++) {
        uint8_t bus = pci_devices[i].bus;
        uint8_t device = pci_devices[i].device;
        uint8_t function = pci_devices[i].function;
        printf("bus %d device %d function %d: vendor 0x%04X, device 0x%04X (%d, %d, %d)\n", bus, device, function, pci_devices[i].vendor_id, pci_devices[i].device_id, pci_devices[i].base_class, pci_devices[i].sub_class, pci_devices[i].interface);
        uint32_t bar = _bus_pci_cfg_read32(bus, device, function, PCI_CFGSPACE_BAR0);
        if (bar) {
            printf("\tBAR0: %08lX\n", bar);
        }
        bar = _bus_pci_cfg_read32(bus, device, function, PCI_CFGSPACE_BAR1);
        if (bar) {
            printf("\tBAR1: %08lX\n", bar);
        }
        bar = _bus_pci_cfg_read32(bus, device, function, PCI_CFGSPACE_BAR2);
        if (bar) {
            printf("\tBAR2: %08lX\n", bar);
        }
        bar = _bus_pci_cfg_read32(bus, device, function, PCI_CFGSPACE_BAR3);
        if (bar) {
            printf("\tBAR3: %08lX\n", bar);
        }
        bar = _bus_pci_cfg_read32(bus, device, function, PCI_CFGSPACE_BAR4);
        if (bar) {
            printf("\tBAR4: %08lX\n", bar);
        }
        bar = _bus_pci_cfg_read32(bus, device, function, PCI_CFGSPACE_BAR5);
        if (bar) {
            printf("\tBAR5: %08lX\n", bar);
        }
    }

    return 0;
}

static int lsdev_handler(struct shell_instance *inst, int argc, char **argv)
{
    struct device *dev = get_first_device();
    while (dev) {
        printf("- %s%d: %s\n", dev->name, dev->id, dev->driver->name);

        dev = dev->next;
    }

    return 0;
}

static int acpi_handler(struct shell_instance *inst, int argc, char **argv)
{
    /* List ACPI Info */
    struct acpi_rsdp *rsdp = acpi_find_rsdp();
    if (!rsdp) {
        return 1;
    }

    printf("RSDP found at 0x%p\n", (void *)rsdp);
    printf("\tVersion: %d", rsdp->revision);
    switch (rsdp->revision) {
        case 0:
            printf(" ACPI 1.0\n");
            break;
        case 2:
            printf(" ACPI 2.0 or upper\n");
            break;
        default:
            printf(" Unknown\n");
            break;
    }
    printf("\tOEM ID: %s\n", rsdp->oem_id);

    struct acpi_rsdt *rsdt = (struct acpi_rsdt *)rsdp->rsdt_address;
    printf("\tRSDT: 0x%p\n", (void *)rsdt);
    printf("\t\tCreator ID: %4s\n", (const char *)&rsdt->header.creator_id);
    printf("\t\tCreator Revision: 0x%08lX\n", rsdt->header.creator_revision);
    printf("\t\tLength: 0x%08lX\n", rsdt->header.length);

    if (rsdp->revision >= 2) {
        printf("\tXSDT: 0x%016llX\n", rsdp->xsdt_address);
    }

    printf("\tTables: \n");
    for (int i = 0; i < (rsdt->header.length - sizeof(rsdt->header)) / sizeof(uint32_t); i++) {
        struct acpi_sdt_header *header = (struct acpi_sdt_header *)(rsdt->table_pointers[i]);
        printf("\t- %4s: 0x%p\n", header->signature, (void *)header);
    }

    struct acpi_fadt *fadt = acpi_find_table(rsdt, ACPI_FADT_SIGNATURE);
    if (fadt) {
        printf("FADT: 0x%p\n", (void *)fadt);

        printf("\tPreferred PM Profile: %d\n", fadt->preferred_pm_profile);
        printf("\tSCI Interrupt: %d\n", fadt->sci_interrupt);
        printf("\tSMI Command Port: %08lX\n", fadt->smi_command_port);

        printf("\tACPI Enable Port: %04X\n", fadt->acpi_enable);
        printf("\tACPI Disable Port: %04X\n", fadt->acpi_disable);

        printf("\tPM1: evt_len=%d, ctl_len=%d\n", fadt->pm1_event_length, fadt->pm1_control_length);
        printf("\tPM1a: evt_blk=0x%08lX, ctl_blk=0x%08lX\n", fadt->pm1a_event_block, fadt->pm1a_control_block);
        printf("\tPM1b: evt_blk=0x%08lX, ctl_blk=0x%08lX\n", fadt->pm1b_event_block, fadt->pm1b_control_block);
        
        printf("\tPM2: ctl_blk=0x%08lX, ctl_len=%d\n", fadt->pm2_control_block, fadt->pm2_control_length);

        printf("\tPM Timer: block=0x%08lX, length=%d\n", fadt->pm_timer_block, fadt->pm_timer_length);

        printf("\tGPE0: block=0x%08lX, length=%d\n", fadt->gpe0_block, fadt->gpe0_length);
        printf("\tGPE1: base=%d, block=0x%08lX, length=%d\n", fadt->gpe1_base, fadt->gpe1_block, fadt->gpe1_length);

        printf("\tCSTATE Control: %02X\n", fadt->cstate_control);
        printf("\tWorst Latency: c2=%u c3=%u\n", fadt->worst_c2_latency, fadt->worst_c3_latency);
        printf("\tCPU Cache Flush: size=%u, stride=%u\n", fadt->flush_size, fadt->flush_stride);
        printf("\tDuty Cycle Setting: offset=%u, width=%u\n", fadt->duty_offset, fadt->duty_width);
        printf("\tRTC Alarm Offset: day=%u, month=%u\n", fadt->day_alarm, fadt->month_alarm);
        printf("\tRTC Century Offset: %u\n", fadt->century);

        if (fadt->header.revision >= 3) {
            printf("\tIA-PC Boot Architecture Flags: %04X\n", fadt->iapc_boot_arch);
        }
    }

    struct acpi_madt *madt = acpi_find_table(rsdt, ACPI_MADT_SIGNATURE);
    if (madt) {
        printf("MADT: 0x%p\n", (void *)madt);

        printf("\tLocal APIC Address: 0x%08lX\n", madt->lapic_address);
        printf("\t8259 PIC Installed: %s\n", (madt->flags & 1) ? "true" : "false");

        union {
            struct madt_entry_header header;
            struct madt_entry_lapic lapic;
            struct madt_entry_ioapic ioapic;
            struct madt_entry_int_src_override int_src_override;
            struct madt_entry_nmi_src nmi_src;
            struct madt_entry_lapic_nmi lapic_nmi;
            struct madt_entry_lapic_addr_override lapic_addr_override;
            struct madt_entry_local_x2apic local_x2apic;
        } *entry = (void *)((uint8_t *)madt + sizeof(*madt));

        while ((ptrdiff_t)entry - (ptrdiff_t)madt < madt->header.length) {
            switch (entry->header.type) {
                case ACPI_MADT_ENTRY_TYPE_LAPIC:
                    printf("\tProcessor Local APIC Entry:\n");
                    printf("\t\tACPI Processor ID: 0x%02X\n", entry->lapic.acpi_processor_id);
                    printf("\t\tLAPIC ID: 0x%02X\n", entry->lapic.apic_id);
                    printf("\t\tFlags: 0x%08lX\n", entry->lapic.flags);
                    break;
                case ACPI_MADT_ENTRY_TYPE_IOAPIC:
                    printf("\tI/O APIC Entry:\n");
                    printf("\t\tID: 0x%02X\n", entry->ioapic.io_apic_id);
                    printf("\t\tAddress: 0x%08lX\n", entry->ioapic.io_apic_address);
                    printf("\t\tGlobal System Interrupt Base: 0x%08lX\n", entry->ioapic.global_system_interrupt_base);
                    break;
                case ACPI_MADT_ENTRY_TYPE_INT_SRC_OVERRIDE:
                    printf("\tInterrupt Source Override Entry:\n");
                    printf("\t\tBus: 0x%02X\n", entry->int_src_override.bus_source);
                    printf("\t\tIRQ: 0x%02X\n", entry->int_src_override.irq_source);
                    printf("\t\tGlobal System Interrupt: 0x%08lX\n", entry->int_src_override.global_system_interrupt);
                    printf("\t\tFlags: 0x%04X\n", entry->int_src_override.flags);
                    break;
                case ACPI_MADT_ENTRY_TYPE_NMI_SRC:
                    printf("\tNMI Source Entry:\n");
                    break;
                case ACPI_MADT_ENTRY_TYPE_LAPIC_NMI:
                    printf("\tLocal APIC NMI Entry:\n");
                    printf("\t\tACPI Processor ID: 0x%02X\n", entry->lapic_nmi.acpi_processor_id);
                    printf("\t\tFlags: 0x%04X\n", entry->lapic_nmi.flags);
                    printf("\t\tLocal APIC LINT#: 0x%02X\n", entry->lapic_nmi.lint);
                    break;
                case ACPI_MADT_ENTRY_TYPE_LAPIC_ADDR_OVERRIDE:
                    printf("\tLocal APIC Address Override Entry:\n");
                    printf("\t\tAddress: 0x%016llX\n", entry->lapic_addr_override.address);
                    break;
                default:
                    break;
            }

            entry = (void *)((uint8_t *)entry + entry->header.length);
        }
    }

    return 0;
}

static float hue2rgb(float p, float q, float t)
{
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f / 2.0f) return q;
    if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
    return p;
}

static void hsl2rgb(float h, uint8_t s_in, uint8_t l_in, uint8_t rgb[3])
{
    if (s_in == 0) {
        rgb[0] = rgb[1] = rgb[2] = 0;
    } else {
        float s = (float)s_in / 255.0f, l = (float)l_in / 255.0f;
        float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
        float p = 2.0f * l - q;
        rgb[0] = hue2rgb(p, q, h + 1.0f / 3.0f) * 255.0f;
        rgb[1] = hue2rgb(p, q, h) * 255;
        rgb[2] = hue2rgb(p, q, h - 1.0f / 3.0f) * 255.0f; 
    }
}

static int testtty_handler(struct shell_instance *inst, int argc, char **argv)
{
    puts("\x1b[0m");
    printf("\x1b[1mbold\x1b[0m\n");
    printf("\x1b[2mdim\x1b[0m\n");
    printf("\x1b[3mitalic\x1b[0m\n");
    printf("\x1b[4munderline\x1b[0m\n");
    printf("\x1b[5mblink slow\x1b[0m\n");
    printf("\x1b[6mblink fast\x1b[0m\n");
    printf("\x1b[7mreversed\x1b[0m\n");
    printf("\x1b[9mstrike\x1b[0m\n");
    printf("\x1b[53moverline\x1b[0m\n");
    for (int i = 0; i < 8; i++) {
        printf("\x1b[%dm@", 30 + i);
    }
    for (int i = 0; i < 8; i++) {
        printf("\x1b[%dm@", 90 + i);
    }
    puts("\x1b[0m\n");
    for (int i = 0; i < 16; i++) {
        printf("\x1b[38;5;%dm@", i);
    }
    puts("\x1b[0m\n");
    for (int i = 16; i < 232; i++) {
        printf("\x1b[38;5;%dm@", i);
        if ((i - 16) % 36 == 35) {
            puts("\x1b[0m\n");
        }
    }
    for (int i = 232; i < 256; i++) {
        printf("\x1b[38;5;%dm@", i);
    }
    puts("\x1b[0m\n");
    for (int g = 0; g < 8; g++) {
        for (int b = 0; b < 8; b++) {
            for (int r = 0; r < 8; r++) {
                printf("\x1b[38;2;%d;%d;%dm@", r * 255 / 7, g * 255 / 7, b * 255 / 7);
            }
            puts("\x1b[0m");
        }
        puts("\x1b[0m\n");
    }
    for (int i = 0; i < 8; i++) {
        printf("\x1b[%dm ", 40 + i);
    }
    for (int i = 0; i < 8; i++) {
        printf("\x1b[%dm ", 100 + i);
    }
    puts("\x1b[0m\n");
    for (int i = 0; i < 16; i++) {
        printf("\x1b[48;5;%dm ", i);
    }
    puts("\x1b[0m\n");
    for (int i = 16; i < 232; i++) {
        printf("\x1b[48;5;%dm ", i);
        if ((i - 16) % 36 == 35) {
            puts("\x1b[0m\n");
        }
    }
    for (int i = 232; i < 256; i++) {
        printf("\x1b[48;5;%dm ", i);
    }
    puts("\x1b[0m\n");
    for (int g = 0; g < 8; g++) {
        for (int b = 0; b < 8; b++) {
            for (int r = 0; r < 8; r++) {
                printf("\x1b[48;2;%d;%d;%dm ", r * 255 / 7, g * 255 / 7, b * 255 / 7);
            }
            puts("\x1b[0m");
        }
        puts("\x1b[0m\n");
    }
    for (int l = 7; l >= 0; l--) {
        for (int h = 0; h < 64; h++) {
            uint8_t rgb[3];
            hsl2rgb((float)h / 63.0f, 255, l * 255 / 7 , rgb);
            printf("\x1b[48;2;%d;%d;%dm ", rgb[0], rgb[1], rgb[2]);
        }
        puts("\x1b[0m\n");
    }
    puts("ㄱ가ㄲ까ㄴ나ㄷ다ㄸ따ㄹ라ㅁ마ㅂ바ㅃ빠ㅅ사ㅆ싸ㅇ아ㅈ자ㅉ짜ㅊ차ㅋ카ㅌ타ㅍ파ㅎ하\n");
    return 0;
}

static int testmouse_handler(struct shell_instance *inst, int argc, char **argv)
{
    struct device *msdev = find_device("mouse0");
    if (!msdev) {
        fprintf(stderr, "%s: cannot find device\n", argv[0]);
        return 1;
    }

    const struct hid_interface *hidif = msdev->driver->get_interface(msdev, "hid");

    struct device *fbdev = find_device("video0");
    if (!fbdev) {
        fprintf(stderr, "%s: cannot find device\n", argv[0]);
        return 1;
    }

    const struct framebuffer_interface *fbif = fbdev->driver->get_interface(fbdev, "framebuffer");

    int width, height;
    uint32_t *framebuffer = fbif->get_framebuffer(fbdev);
    fbif->get_mode(fbdev, &width, &height, NULL);

    int xpos = width / 2, ypos = height / 2, should_exit = 0;
    uint16_t key, flags;

    puts("\x1b[3J\x1b[0;0f\x1b[?25lclose");

    while (!should_exit) {
        hidif->wait_event(msdev);
        if (hidif->poll_event(msdev, &key, &flags)) {
            continue;
        }

        switch (flags & KEY_FLAG_TYPEMASK) {
            case 0:
                if (key == KEY_MOUSEBTNL && !(flags & KEY_FLAG_BREAK)) {
                    if (xpos < 40 && ypos < 16) {
                        should_exit = 1;
                    }
                }
                break;
            case KEY_FLAG_XMOVE:
                if ((flags & KEY_FLAG_NEGATIVE)) {
                    if (xpos < key) {
                        xpos = 0;
                    } else {
                        xpos -= key;
                    }
                } else if (!(flags & KEY_FLAG_NEGATIVE)) {
                    if (width <= xpos + key) {
                        xpos = width - 1;
                    } else {
                        xpos += key;
                    }
                }
                break;
            case KEY_FLAG_YMOVE:
                if ((flags & KEY_FLAG_NEGATIVE)) {
                    if (height <= ypos + key) {
                        ypos = height - 1;
                    } else {
                        ypos += key;
                    }
                } else if (!(flags & KEY_FLAG_NEGATIVE)) {
                    if (ypos < key) {
                        ypos = 0;
                    } else {
                        ypos -= key;
                    }
                }

                _i686_disable_interrupt();
                framebuffer[ypos * width + xpos] = 0xFFFFFF;
                fbif->invalidate(fbdev, xpos, ypos, xpos, ypos);
                fbif->flush(fbdev);
                fbif->present(fbdev);
                _i686_enable_interrupt();
                break;
            default:
                break;
        }
    }

    puts("\x1b[3J\x1b[?25h\x1b[0;0f");

    return 0;
}

static int read_handler(struct shell_instance *inst, int argc, char **argv)
{
    char *next;
    uint32_t addr = strtoul(argv[1], &next, 16);
    int32_t count = strtol(next + 1, NULL, 10);
    if (!count) {
        count = 1;
    }

    while (count > 0) {
        printf("%08lX: ", addr);
        for (int i = 0; i < 16 && count > 0; i++) {
            printf("%02X ", *(uint8_t *)addr);

            addr++;
            count--;
        }
        printf("\n");
    }
    return 0;
}

static int write_handler(struct shell_instance *inst, int argc, char **argv)
{
    char *next;
    uint32_t addr = strtoul(argv[1], &next, 16);
    uint8_t value = strtol(next + 1, NULL, 16);

    *(uint8_t *)addr = value;
    
    return 0;
}

static int in_handler(struct shell_instance *inst, int argc, char **argv)
{
    char *next;
    uint16_t addr = strtol(argv[1], &next, 16);
    
    if (!*next) return 1;

    switch (next[1]) {
        case 'b':
        case 'B':
            printf("%02X\n", _i686_in8(addr));
            break;
        case 'w':
        case 'W':
            printf("%04X\n", _i686_in16(addr));
            break;
        case 'd':
        case 'D':
            printf("%08lX\n", _i686_in32(addr));
            break;
        default:
            return 1;
    }

    return 0;
}

static int out_handler(struct shell_instance *inst, int argc, char **argv)
{
    char *next;
    uint16_t addr = strtol(argv[1], &next, 16);
    uint32_t value = strtoul(next + 1, &next, 16);

    if (!*next) return 1;

    switch (next[1]) {
        case 'b':
        case 'B':
            _i686_out8(addr, value);
            break;
        case 'w':
        case 'W':
            _i686_out16(addr, value);
            break;
        case 'd':
        case 'D':
            _i686_out32(addr, value);
            break;
        default:
            return 1;
    }
    
    return 0;
}

static int readblk_handler(struct shell_instance *inst, int argc, char **argv)
{
    char *next;
    lba_t lba = strtoull(argv[1], &next, 16);

    if (!*next) {
        fprintf(stderr, "usage: %s [lba] [device]\n", argv[0]);
        return 1;
    }

    struct device *blkdev = find_device(next + 1);
    if (!blkdev) {
        fprintf(stderr, "%s: could not find device\n", argv[0]);
        return 1;
    }
    const struct block_interface *blkif = blkdev->driver->get_interface(blkdev, "block");

    uint8_t buf[512];
    blkif->read(blkdev, lba, buf, 1);

    int count = sizeof(buf);
    uint8_t *addr = buf;
    while (count > 0) {
        for (int i = 0; i < 16 && count > 0; i++) {
            printf("%02X ", *(uint8_t *)addr);

            addr++;
            count--;
        }
        printf("\n");
    }

    return 0;
}

static int mount_handler(struct shell_instance *inst, int argc, char **argv)
{
    char *fsname = NULL;
    for (int i = 0; argv[1][i]; i++) {
        if (argv[1][i] == ' ') {
            fsname = &argv[1][i];
            break;
        }
    }
    if (!fsname || !fsname[1]) {
        fprintf(stderr, "usage: %s [device] [fs_name]\n", argv[0]);
        return 1;
    }

    *fsname++ = '\0';

    struct device *blkdev = find_device(argv[1]);
    if (!blkdev) {
        fprintf(stderr, "%s: could not find device\n", argv[0]);
        return 1;
    }
    
    return fs_auto_mount(blkdev, fsname);
}

static int lsfs_handler(struct shell_instance *inst, int argc, char **argv)
{
    struct filesystem *current = get_first_filesystem();
    
    while (current) {
        printf("%s: %s\n", current->name, current->driver->name);

        current = current->next;
    }

    return 0;
}

static int chfs_handler(struct shell_instance *inst, int argc, char **argv)
{
    struct filesystem *newfs = find_filesystem(argv[1]);
    if (!newfs) {
        fprintf(stderr, "%s: could not find filesystem\n", argv[0]);
        return 1;
    }

    if (inst->working_dir) inst->working_dir->fs->driver->close_directory(inst->working_dir);
    inst->working_dir = newfs->driver->open_root_directory(newfs);
    inst->working_dir_path[0] = '/';
    inst->working_dir_path[1] = '\0';
    inst->fs = newfs;

    return 0;
}

static int lsfile_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (!inst->fs) {
        fprintf(stderr, "%s: filesystem not selected\n", argv[0]);
        return 1;
    }

    inst->working_dir->fs->driver->rewind_directory(inst->working_dir);

    struct fs_directory_entry direntry;
    while (!inst->working_dir->fs->driver->iter_directory(inst->working_dir, &direntry)) {
        if (strcmp(direntry.name, ".") == 0 || strcmp(direntry.name, "..") == 0) {
            continue;
        }

        printf("%8llu  %s\n", direntry.size, direntry.name);
    }

    return 0;
}

static int chdir_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (!inst->fs) {
        fprintf(stderr, "%s: filesystem not selected\n", argv[0]);
        return 1;
    }

    struct fs_directory *newdir = inst->working_dir->fs->driver->open_directory(inst->working_dir, argv[1]);
    if (!newdir) {
        fprintf(stderr, "%s: failed to open directory\n", argv[0]);
        return 1;
    }
    inst->working_dir->fs->driver->close_directory(inst->working_dir);
    
    inst->working_dir = newdir;
    if (strcmp(argv[1], ".") == 0) {
    } else if (strcmp(argv[1], "..") == 0) {
        int prev_path_len = strlen(inst->working_dir_path);
        char *end = strrchr(inst->working_dir_path, '/');
        if (inst->working_dir_path == end) {
            end[1] = '\0';
        } else {
            *end = '\0';
        }
    } else {
        int prev_path_len = strlen(inst->working_dir_path);
        int dirname_len = strlen(argv[1]);
        if (prev_path_len == 1) prev_path_len--;
        inst->working_dir_path[prev_path_len] = '/';
        strcpy(inst->working_dir_path + prev_path_len + 1, argv[1]);
        inst->working_dir_path[prev_path_len + dirname_len + 1] = '\0';
    }

    return 0;
}

static int readfile_handler(struct shell_instance *inst, int argc, char **argv)
{
    char path[PATH_MAX];
    if (path_is_absolute(argv[1])) {
        strncpy(path, argv[1], sizeof(path));
    } else {
        snprintf(path, sizeof(path), "%s:%s", inst->fs->name, inst->working_dir_path);
        path_join(path, sizeof(path), argv[1]);

        if (!inst->fs) {
            fprintf(stderr, "%s: filesystem not selected\n", argv[0]);
            return 1;
        }
    }

    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "%s: failed to open file\n", argv[0]);
        return 1;
    }

    char buf[512];
    int read_count;
    while ((read_count = fread(buf, 1, sizeof(buf), fp)) > 0) {
        printf("%*s", read_count, buf);
    }
    printf("\n");
    
    fclose(fp);

    return 0;
}

static int dispimg_handler(struct shell_instance *inst, int argc, char **argv)
{
    char path[PATH_MAX];
    if (path_is_absolute(argv[1])) {
        strncpy(path, argv[1], sizeof(path));
    } else {
        snprintf(path, sizeof(path), "%s:%s", inst->fs->name, inst->working_dir_path);
        path_join(path, sizeof(path), argv[1]);

        if (!inst->fs) {
            fprintf(stderr, "%s: filesystem not selected\n", argv[0]);
            return 1;
        }
    }

    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "%s: failed to open file\n", argv[0]);
        return 1;
    }

    struct device *fbdev = find_device("video0");
    if (!fbdev) {
        fprintf(stderr, "%s: cannot find device\n", argv[0]);
        return 1;
    }

    const struct framebuffer_interface *fbif = fbdev->driver->get_interface(fbdev, "framebuffer");

    int width, height;
    uint32_t *framebuffer = fbif->get_framebuffer(fbdev);
    fbif->get_mode(fbdev, &width, &height, NULL);

    struct bmp_header {
        char signature[2];
        uint32_t file_size;
        uint8_t unused[4];
        uint32_t bitmap_offset;
    } __attribute__((packed));

    struct bmp_dibheader_core {
        uint32_t header_size;
        uint32_t width;
        uint32_t height;
        uint16_t color_planes;
        uint16_t bpp;
    } __attribute__((packed));

    struct bmp_header header;
    fread(&header, sizeof(header), 1, fp);

    if (header.signature[0] != 'B' || header.signature[1] != 'M') {
        fprintf(stderr, "%s: not a BMP file\n", argv[0]);
        fclose(fp);
        return 1;
    }

    struct bmp_dibheader_core dibheader;
    fread(&dibheader, sizeof(dibheader), 1, fp);

    printf("bmp file size: %lu bytes, bitmap offset %lu\n", header.file_size, header.bitmap_offset);
    printf("width: %lu, height: %lu, bpp: %u\n", dibheader.width, dibheader.height, dibheader.bpp);

    fseek(fp, header.bitmap_offset, SEEK_SET);
    for (int y = 0; y < dibheader.height; y++) {
        for (int x = 0; x < dibheader.width; x++) {
            uint32_t buf;
            fread(&buf, dibheader.bpp / 8, 1, fp);
            framebuffer[(dibheader.height - y - 1) * width + x] = buf;
        }
    }
    fbif->invalidate(fbdev, 0, 0, width - 1, height - 1);
    fbif->flush(fbdev);
    fbif->present(fbdev);
    
    fclose(fp);

    char ch;
    fread(&ch, sizeof(ch), 1, stdin);

    puts("\x1b[3J\x1b[0;0f");

    return 0;
}

static int drvinfo_handler(struct shell_instance *inst, int argc, char **argv)
{
    uint8_t drive = strtol(argv[1], NULL, 16);
    enum bios_drive_type drive_type;
    struct chs drive_geometry;

    int err = _pc_bios_get_drive_params(drive, &drive_type, &drive_geometry, NULL);
    if (err) return err;

    static const char *const drive_types[] = {
        "Hard disk",
        "360k",
        "1.2M",
        "720k",
        "1.44M",
        "2.88M",
        "2.88M",
    };

    printf("Type: %s\n", drive_types[drive_type]);
    printf("Geometry: cylinder=%d, head=%d, sector=%d\n", drive_geometry.cylinder, drive_geometry.head, drive_geometry.sector);

    struct bios_extended_drive_params params;
    enum bios_edd_version edd_version;
    err = _pc_bios_check_ext(drive, &edd_version, NULL);
    if (err) {
        printf("EDD Not supported\n");
        return 0;
    }

    switch (edd_version) {
        case BIOS_EDD_1_X:
            printf("EDD Version: 1.x\n");
            break;
        case BIOS_EDD_2_0:
            printf("EDD Version: 2.0\n");
            break;
        case BIOS_EDD_2_1:
            printf("EDD Version: 2.1\n");
            break;
        case BIOS_EDD_3_0:
            printf("EDD Version: 3.0\n");
            break;
        default:
            printf("Unknown EDD Version: %02X\n", edd_version);
            break;
    }


    err = _pc_bios_get_drive_params_ext(drive, &params);
    if (err) return err;

    if (edd_version >= BIOS_EDD_1_X) {
        printf("Flags: %04X\n", params.flags);
        printf("Geometry: cylinder=%lu, head=%lu, sector=%lu\n", params.geom_cylinders, params.geom_heads, params.geom_sectors);
        printf("Bytes per sector: %u\n", params.bytes_per_sector);
        printf("Total sectors: %llu\n", params.total_sectors);
    }

    if (edd_version >= BIOS_EDD_3_0 && params.device_path_signature == 0xBEDD) {
        printf("Host bus: %4s\n", params.host_bus);
        printf("Interface: %8s\n", params.interface);
    }

    return 0;
}

static int lsint_handler(struct shell_instance *inst, int argc, char **argv)
{
    for (int i = 0; i < 256; i++) {
        farptr_t ptr = ((farptr_t *)0)[i];
        printf("Interrupt %u: %04X:%04X\n", i, ptr.segment, ptr.offset);
    }

    return 0;
}

static int chainload_handler(struct shell_instance *inst, int argc, char **argv)
{
    uint8_t drive = argc < 2 ? _pc_boot_drive : strtol(argv[1], NULL, 16);
    printf("Booting into drive 0x%02X...\n", drive);
    return _pc_chainload(drive);
}

static int bootnext_handler(struct shell_instance *inst, int argc, char **argv)
{
    _pc_bios_bootnext();
    return 1;
}

static int usefont_handler(struct shell_instance *inst, int argc, char **argv)
{
    return font_use(argc < 2 ? NULL : argv[1]);
}

static int jump_handler(struct shell_instance *inst, int argc, char **argv)
{
    uint32_t addr = strtol(argv[1], NULL, 16);
    return ((int (*)(void))addr)();
}

static int help_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (argc > 1) {
        int found = 0;
        for (int i = 0; i < ARRAY_SIZE(commands); i++) {
            if (strncmp(commands[i].name, argv[1], strlen(commands[i].name)) == 0) {
                printf("%s\n", commands[i].help_message);
                found = 1;
            }
        }

        if (!found) {
            puts("Command not found\n");
        }
    } else {
        for (int i = 0; i < ARRAY_SIZE(commands); i++) {
            printf("\x1b[1m\x1b[94m%s\x1b[0m: %s\n", commands[i].name, commands[i].help_message);
        }
    }
    return 0;
}

__attribute__((noreturn))
static int reboot_handler(struct shell_instance *inst, int argc, char **argv)
{
    _i686_pc_reboot();
}

__attribute__((noreturn))
static int poweroff_handler(struct shell_instance *inst, int argc, char **argv)
{
    _i686_pc_poweroff();
}

static int exit_handler(struct shell_instance *inst, int argc, char **argv)
{
    return 0;
}

int handle_command(struct shell_instance *inst, char *cmdline, int len)
{
    int cmd_len = 0, noarg = 0;
    for (int i = 0; cmdline[i] != ' ' && i < len; i++) {
        cmd_len++;
    }
    if (!cmdline[cmd_len]) noarg = 1;
    cmdline[cmd_len] = 0;

    for (int i = 0; i < ARRAY_SIZE(commands); i++) {
        if (strcmp(commands[i].name, cmdline) == 0) {
            char *argv[] = { cmdline, cmdline + cmd_len + 1 };
            return commands[i].handler(inst, noarg ? 1 : 2, argv);
        }

    }

    printf("command not found: %*s\n", cmd_len + 1, cmdline);
    return 1;
}

int shell_handler(struct shell_instance *inst, int argc, char **argv)
{
    struct shell_instance new_inst = {
        .fs = NULL,
        .working_dir = NULL,
        .working_dir_path = { 0, },
    };

    char buf[512];
    int result = 0;

    for (;;) {
        if (result) {
            printf("(%d) ", result);
        }
        if (new_inst.fs) {
            printf("%s:%s", new_inst.fs->name, new_inst.working_dir_path);
        }
        printf("> ");
        int len = readline(buf, sizeof(buf));
        if (len == 0) {
            result = 0;
            continue;
        }
        if (strncmp(buf, "exit", sizeof(buf)) == 0) {
            break;
        }
        result = handle_command(&new_inst, buf, len);
    }

    return 0;
}

void start_shell(void)
{
    printf("\x1b[3J\x1b[0;0f\x1b[?25h\x1b[0m");

    struct shell_instance inst = {
        .fs = NULL,
        .working_dir = NULL,
        .working_dir_path = { 0, },
    };

    char buf[6];
    strncpy(buf, "shell", 6);

    for (;;) {
        shell_handler(&inst, 1, (char **)&buf);
    }

    _i686_disable_interrupt();
}
