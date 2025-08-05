#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <macros.h>
#include <stdlib.h>
#include <cpuid.h>
#include <ctype.h>
#include <getopt.h>

#include <device/driver.h>
#include <interface/char.h>
#include <interface/console.h>
#include <interface/framebuffer.h>
#include <bus/pci/scan.h>
#include <acpi/rsdp.h>
#include <acpi/rsdt.h>
#include <acpi/fadt.h>
#include <asm/io.h>
#include <asm/bios/video.h>
#include <asm/bios/disk.h>
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
static int chainload_handler(int argc, char **argv);
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
    { "chainload", chainload_handler, "Boot from other disk" },
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

    int opt;
    getopt_init();
    while ((opt = getopt_long(argc, argv, "l", opts, NULL)) != -1) {
        switch (opt) {
            case 'l': {
                // struct device *fbdev = find_device("video");
                // const struct framebuffer_interface *fbif = fbdev->driver->get_interface(fbdev, "framebuffer");

                struct vbe_controller_info vbe_info;
                _pc_bios_get_vbe_controller_info(&vbe_info);
            
                uint16_t *mode_list = (uint16_t*)((vbe_info.video_modes.segment << 4) + vbe_info.video_modes.offset);
                struct vbe_video_mode_info vbe_mode_info;
                uint16_t mode = 0xFFFF;
                printf("Video Modes:\r\n");
                for (int i = 0; mode_list[i] != 0xFFFF; i++) {
                    _pc_bios_get_vbe_video_mode_info(mode_list[i], &vbe_mode_info);
                    printf("Mode %04X:\tw=%d\th=%d\tbpp=%d\taddr=%08lX\tattr=%04X\tmm=%d\r\n", mode_list[i], vbe_mode_info.width, vbe_mode_info.height, vbe_mode_info.bpp, vbe_mode_info.framebuffer, vbe_mode_info.attributes, vbe_mode_info.memory_model);
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
    _pc_bios_get_vbe_video_mode_info(mode, &vbe_mode_info);

    struct device *framebuffer_device = find_device("video");
    const struct framebuffer_interface *framebuffer = framebuffer_device->driver->get_interface(framebuffer_device, "framebuffer");
    
    struct device *chrvideo_device = find_device("chrvideo");
    const struct console_interface *console = chrvideo_device->driver->get_interface(chrvideo_device, "console");

    framebuffer->set_mode(framebuffer_device, vbe_mode_info.width, vbe_mode_info.height, vbe_mode_info.bpp);
    console->set_dimension(chrvideo_device, vbe_mode_info.width / 8, vbe_mode_info.height / 16);

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
        printf("bus %d device %d function %d: vendor 0x%04X, device 0x%04X (%d, %d, %d)\n", pci_devices[i].bus, pci_devices[i].device, pci_devices[i].function, pci_devices[i].vendor_id, pci_devices[i].device_id, pci_devices[i].base_class, pci_devices[i].sub_class, pci_devices[i].interface);
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

    printf("RSDP found at 0x%08lX\n", (uint32_t)rsdp);
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
    printf("\tRSDT: 0x%08lX\n", (uint32_t)rsdt);
    printf("\t\tCreator ID: %4s\n", (const char *)&rsdt->header.creator_id);
    printf("\t\tCreator Revision: 0x%08lX\n", rsdt->header.creator_revision);
    printf("\t\tLength: 0x%08lX\n", rsdt->header.length);

    printf("\tXSDT: 0x%016llX\n", rsdp->xsdt_address);

    printf("\tTables: \n");
    for (int i = 0; i < (rsdt->header.length - sizeof(rsdt->header)) / sizeof(uint32_t); i++) {
        struct acpi_sdt_header *header = (struct acpi_sdt_header *)(rsdt->table_pointers[i]);
        printf("\t- %4s\n", header->signature);
    }

    struct acpi_fadt *fadt = find_fadt(rsdt);
    if (fadt) {
        printf("FADT: 0x%08lX\n", (uint32_t)fadt);

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
    }

    return 0;
}

extern int _pc_bios_chainload(uint8_t drive);

static int chainload_handler(int argc, char **argv)
{
    uint8_t drive = strtol(argv[1], NULL, 16);
    return _pc_bios_chainload(drive);
}

static int help_handler(int argc, char **argv)
{
    for (int i = 0; i < ARRAY_SIZE(commands); i++) {
        printf("\x1b[1m\x1b[94m%s\x1b[0m: %s\n", commands[i].name, commands[i].help_message);
    }
    return 0;
}

__attribute__((noreturn))
static int reboot_handler(int argc, char **argv)
{
    uint8_t status;
    do {
        status = _i686_in8(0x0064);
    } while (status & 0x02);
    _i686_out8(0x0064, 0xFE);

    panic("Reboot failed");
}

__attribute__((noreturn))
static int poweroff_handler(int argc, char **argv)
{
    _i686_out16(0xB004, 0x2000);
    _i686_out16(0x0604, 0x2000);
    _i686_out16(0x4004, 0x3400);

    panic("Poweroff failed");
}

static int exit_handler(int argc, char **argv)
{
    return 0;
}

int handle_command(char *cmdline, int len)
{
    int cmd_len = 0;
    for (int i = 0; cmdline[i] != ' ' && i < len; i++) {
        cmd_len = i;
    }

    for (int i = 0; i < ARRAY_SIZE(commands); i++) {
        if (strncmp(commands[i].name, cmdline, strlen(commands[i].name)) == 0) {
            char *argv[] = { cmdline, cmdline[cmd_len - 1] ? cmdline + cmd_len + 2 : cmdline + cmd_len + 1 };
            return commands[i].handler(2, argv);
        }

    }

    printf("command not found: %*s\n", cmd_len + 1, cmdline);
    return 0;
}

void main(void)
{
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

