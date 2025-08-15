#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <macros.h>
#include <stdlib.h>
#include <cpuid.h>
#include <ctype.h>
#include <getopt.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <interface/char.h>
#include <interface/console.h>
#include <interface/framebuffer.h>
#include <bus/pci/scan.h>
#include <acpi/rsdp.h>
#include <acpi/rsdt.h>
#include <acpi/fadt.h>
#include <asm/bios/video.h>
#include <asm/bios/disk.h>
#include <asm/reboot.h>
#include <asm/poweroff.h>
#include <asm/pci/cfgspace.h>
#include <asm/bootinfo.h>
#include <core/panic.h>

static int readline(char *buf, int len)
{
    int cur = 0;
    char ch;
    do {
        fread(&ch, 1, 1, stdin);
        switch (ch) {
            case '\r':
                printf("\n");
                break;
            case '\b':
                if (cur < 1) {
                    break;
                }
                cur--;
                printf("\x7F");
                break;
            default:
                if (cur >= len - 1) {
                    break;
                }
                printf("%c", ch);
                buf[cur++] = ch;
                break;
        }
    } while (ch != '\r');

    buf[cur] = 0;
    return cur;
}

struct command {
    const char *name;
    int (*handler)(int argc, char **argv);
    const char *help_message;
};

static int echo_handler(int argc, char **argv);
static int vmode_handler(int argc, char **argv);
static int cpuid_handler(int argc, char **argv);
static int lspci_handler(int argc, char **argv);
static int acpi_handler(int argc, char **argv);
static int test_handler(int argc, char **argv);
static int read_handler(int argc, char **argv);
static int write_handler(int argc, char **argv);
static int chainload_handler(int argc, char **argv);
static int drvinfo_handler(int argc, char **argv);
static int jump_handler(int argc, char **argv);
static int help_handler(int argc, char **argv);
static int reboot_handler(int argc, char **argv);
static int poweroff_handler(int argc, char **argv);
static int exit_handler(int argc, char **argv);

static const struct command commands[] = {
    { "echo", echo_handler, "Write arguments to the stanard output" },
    { "vmode", vmode_handler, "List or set video mode" },
    { "cpuid", cpuid_handler, "Invoke CPUID" },
    { "lspci", lspci_handler, "List PCI devices" },
    { "acpi", acpi_handler, "Show ACPI informations" },
    { "test", test_handler, "TEST" },
    { "read", read_handler, "Read value from memory" },
    { "write", write_handler, "Write value to memory" },
    { "chainload", chainload_handler, "Boot from other disk" },
    { "drvinfo", drvinfo_handler, "Show drive information" },
    { "jump", jump_handler, "Jump to address" },
    { "help", help_handler, "Print this message" },
    { "reboot", reboot_handler, "Reboot the computer" },
    { "poweroff", poweroff_handler, "Power off the computer" },
    { "exit", exit_handler, "Exit from shell" },
};

static int echo_handler(int argc, char **argv)
{
    if (argc < 2) return 0;

    char *message = argv[1];
    printf("%s\n", argv[1]);

    return 0;
}

static int vmode_handler(int argc, char **argv)
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
            
                uint16_t *mode_list = (uint16_t*)((vbe_info.video_modes.segment << 4) + vbe_info.video_modes.offset);
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

    struct device *fbdev = find_device("video");
    struct device *condev = find_device("console");
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

static int cpuid_handler(int argc, char **argv)
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

static int lspci_handler(int argc, char **argv)
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

static struct acpi_fadt *find_fadt(struct acpi_rsdt *rsdt)
{
    for (int i = 0; i < (rsdt->header.length - sizeof(rsdt->header)) / sizeof(uint32_t); i++) {
        struct acpi_sdt_header *header = (struct acpi_sdt_header *)(rsdt->table_pointers[i]);
        if (strncmp(header->signature, ACPI_FADT_SIGNATURE, 4) == 0) {
            return (struct acpi_fadt *)header;
        }
    }
    return NULL;
}

static int acpi_handler(int argc, char **argv)
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

    struct acpi_fadt *fadt = find_fadt(rsdt);
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

static void hsl2rgb(const uint8_t hsl[3], uint8_t rgb[3])
{
    if (hsl[1] == 0) {
        rgb[0] = rgb[1] = rgb[2] = 0;
    } else {
        float h = (float)hsl[0] / 255.0f, s = (float)hsl[1] / 255.0f, l = (float)hsl[2] / 255.0f;
        float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
        float p = 2.0f * l - q;
        rgb[0] = hue2rgb(p, q, h + 1.0f / 3.0f) * 255.0f;
        rgb[1] = hue2rgb(p, q, h) * 255;
        rgb[2] = hue2rgb(p, q, h - 1.0f / 3.0f) * 255.0f; 
    }
}

static int test_handler(int argc, char **argv)
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
            uint8_t hsl[3] = { h * 255 / 63, 255, l * 255 / 7 }, rgb[3];
            hsl2rgb(hsl, rgb);
            printf("\x1b[48;2;%d;%d;%dm ", rgb[0], rgb[1], rgb[2]);
        }
        puts("\x1b[0m\n");
    }
    return 0;
}

static int read_handler(int argc, char **argv)
{
    char *next;
    uint32_t addr = strtol(argv[1], &next, 16);
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

static int write_handler(int argc, char **argv)
{
    char *next;
    uint32_t addr = strtol(argv[1], &next, 16);
    uint8_t value = strtol(next + 1, NULL, 16);

    *(uint8_t *)addr = value;
    
    return 0;
}

static int drvinfo_handler(int argc, char **argv)
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

extern int _pc_bios_chainload(uint8_t drive);

static int chainload_handler(int argc, char **argv)
{
    uint8_t drive = argc < 2 ? _pc_boot_drive : strtol(argv[1], NULL, 16);
    printf("Booting into drive 0x%02X...\n", drive);
    return _pc_bios_chainload(drive);
}

static int jump_handler(int argc, char **argv)
{
    uint32_t addr = strtol(argv[1], NULL, 16);
    return ((int (*)(void))addr)();
}

static int help_handler(int argc, char **argv)
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
static int reboot_handler(int argc, char **argv)
{
    _i686_pc_reboot();
}

__attribute__((noreturn))
static int poweroff_handler(int argc, char **argv)
{
    _i686_pc_poweroff();
}

static int exit_handler(int argc, char **argv)
{
    return 0;
}

int handle_command(char *cmdline, int len)
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
            return commands[i].handler(noarg ? 1 : 2, argv);
        }

    }

    printf("command not found: %*s\n", cmd_len + 1, cmdline);
    return 1;
}

extern int __text_start;
extern int __data_start;
extern int __bss_start;

void main(void)
{
    struct device *fbdev = find_device("video");
    const struct framebuffer_interface *fbi = fbdev->driver->get_interface(fbdev, "framebuffer");
    fbi->set_mode(fbdev, 640, 480, 24);

    struct device *vcdev = mm_allocate_clear(1, sizeof(struct device));
    struct device_ref *fbdev_ref = create_device_ref(NULL);
    fbdev_ref->dev = fbdev;
    vcdev->reference = fbdev_ref;
    vcdev->driver = find_driver("vconsole");
    register_device(vcdev);
    const struct console_interface *vci = vcdev->driver->get_interface(vcdev, "console");
    vci->set_dimension(vcdev, 80, 30);

    struct device *ttydev = mm_allocate_clear(1, sizeof(struct device));
    struct device_ref *vcdev_ref = create_device_ref(NULL);
    vcdev_ref->dev = vcdev;
    ttydev->reference = vcdev_ref;
    ttydev->driver = find_driver("ansiterm");
    register_device(ttydev);

    freopen_device(stdout, "tty");
    freopen_device(stderr, "tty");
    freopen_device(stdin, "kbd");

    printf("\x1b[3J\x1b[0;0fEMOS Installer CD-ROM\n");
    printf("=====================\n");

    printf(".text=0x%p, .data=0x%p, .bss=0x%p\n", (void *)&__text_start, (void *)&__data_start, (void *)&__bss_start);

    char buf[512];
    int result = 0;

    for (;;) {
        if (result) {
            printf("(%d) > ", result);
        } else {
            printf("> ");
        }
        int len = readline(buf, sizeof(buf));
        if (len == 0) {
            result = 0;
            continue;
        }
        result = handle_command(buf, len);
    }
}

