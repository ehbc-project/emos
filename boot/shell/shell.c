#include "shell.h"

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <macros.h>
#include <stdlib.h>
#include <ctype.h>
#include <getopt.h>
#include <path.h>

#include <zlib.h>

#include <cleanup.h>
#include <mm/mm.h>
#include <device/driver.h>
#include <interface/char.h>
#include <interface/console.h>
#include <interface/framebuffer.h>
#include <interface/block.h>
#include <interface/hid.h>
#include <bus/pci/scan.h>
#include <bus/pci/class.h>
#include <bus/pci/cfgspace.h>
#include <uacpi/acpi.h>
#include <uacpi/kernel_api.h>
#include <uacpi/tables.h>
#include <asm/cpuid.h>
#include <asm/bios/video.h>
#include <asm/bios/disk.h>
#include <asm/bios/misc.h>
#include <asm/reboot.h>
#include <asm/poweroff.h>
#include <asm/pci/cfgspace.h>
#include <asm/bootinfo.h>
#include <sys/io.h>
#include <asm/breakpoint.h>
#include <asm/interrupt.h>
#include <asm/pic.h>
#include <asm/time.h>
#include <asm/boot.h>
#include <asm/isr.h>
#include <debug.h>
#include <fs/driver.h>
#include <font.h>
#include <elf.h>

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
static int clear_handler(struct shell_instance *inst, int argc, char **argv);
static int vmode_handler(struct shell_instance *inst, int argc, char **argv);
static int dispinfo_handler(struct shell_instance *inst, int argc, char **argv);
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
static int crc32_handler(struct shell_instance *inst, int argc, char **argv);
static int dispimg_handler(struct shell_instance *inst, int argc, char **argv);
static int lsint_handler(struct shell_instance *inst, int argc, char **argv);
static int chainload_handler(struct shell_instance *inst, int argc, char **argv);
static int bootnext_handler(struct shell_instance *inst, int argc, char **argv);
static int drvinfo_handler(struct shell_instance *inst, int argc, char **argv);
static int usefont_handler(struct shell_instance *inst, int argc, char **argv);
static int jump_handler(struct shell_instance *inst, int argc, char **argv);
static int boot_handler(struct shell_instance *inst, int argc, char **argv);
static int help_handler(struct shell_instance *inst, int argc, char **argv);
static int reboot_handler(struct shell_instance *inst, int argc, char **argv);
static int poweroff_handler(struct shell_instance *inst, int argc, char **argv);
static int exit_handler(struct shell_instance *inst, int argc, char **argv);
static int shell_handler(struct shell_instance *inst, int argc, char **argv);

static const struct command commands[] = {
    { "acpi", acpi_handler, "Show ACPI informations" },
    { "boot", boot_handler, "Boot from file" },
    { "bootnext", bootnext_handler, "Boot from next device" },
    { "cd", chdir_handler, "Change working directory" },
    { "cf", chfs_handler, "Change filesystem" },
    { "chainload", chainload_handler, "Boot from other disk" },
    { "chdir", chdir_handler, "Change working directory" },
    { "chfs", chfs_handler, "Change filesystem" },
    { "clear", clear_handler, "Clear console" },
    { "cpuid", cpuid_handler, "Invoke CPUID" },
    { "crc32", crc32_handler, "Get CRC32 checksum of a file" },
    { "drvinfo", drvinfo_handler, "Show drive information" },
    { "dispimg", dispimg_handler, "Display a .bmp image file" },
    { "dispinfo", dispinfo_handler, "Get information of the display" },
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

    for (int i = 1; i < argc; i++) {
        fputs(argv[i], stdout);
        if (i != argc - 1) {
            putc(' ', stdout);
        }
    }
    putc('\n', stdout);

    return 0;
}

static int clear_handler(struct shell_instance *inst, int argc, char **argv)
{
    printf("\x1b[3J\x1b[0;0f");

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
                    printf("Mode %03X: w=%4d h=%4d bpp=%2d addr=%08lX attr=%04X mm=%s\r\n",
                        mode_list[i],
                        vbe_mode_info.width, vbe_mode_info.height, vbe_mode_info.bpp,
                        vbe_mode_info.framebuffer, vbe_mode_info.attributes, memory_models[vbe_mode_info.memory_model]
                    );
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

static int dispinfo_handler(struct shell_instance *inst, int argc, char **argv)
{
    struct edid buf;
    _pc_bios_get_vbe_edid(0, 0, &buf);

    uint16_t manufacturer_id = ((buf.manufacturer_id & 0x00FF) << 8) | ((buf.manufacturer_id & 0xFF00) >> 8);

    printf("Manufacturer: %c%c%c\n", 'A' + ((manufacturer_id & 0x7C00) >> 10), 'A' + ((manufacturer_id & 0x03E0) >> 5), 'A' + (manufacturer_id & 0x001F));

    printf("EDID Version: %02X\n", buf.edid_version);
    printf("EDID Revision: %02X\n", buf.edid_revision);

    for (int i = 0; i < 4; i++) {
        int w = buf.detailed_timings[0].hactive | ((int)(buf.detailed_timings[0].hactive_hblank & 0xF0) << 4);
        int h = buf.detailed_timings[0].vactive | ((int)(buf.detailed_timings[0].vactive_vblank & 0xF0) << 4);

        printf("Preferred resolution #%d: %dx%d\n", i, w, h);
    }


    return 0;
}

static int cpuid_handler(struct shell_instance *inst, int argc, char **argv)
{
    uint32_t max_param;

    uint32_t eax, ebx, ecx, edx;
    _i686_cpuid(0, &eax, &ebx, &ecx, &edx);

    max_param = eax;
    uint32_t vendor_str[3] = { ebx, edx, ecx };
    printf("Vendor String: %12s\n", (char *)vendor_str);

    if (max_param >= 1) {
        _i686_cpuid(1, &eax, &ebx, &ecx, &edx);

        printf("Processor Type: %1lX\n", (eax & 0x00003000) >> 12);
        printf("Model ID: %02lX\n", ((eax & 0x000F0000) >> 12) | ((eax & 0x000000F0) >> 4));
        printf("Family ID: %03lX\n", ((eax & 0x0FF00000) >> 16) | ((eax & 0x00000F00) >> 8));
        printf("Stepping ID: %1lX\n", eax & 0x0000000F);

        printf("Brand Index: %02lX\n", ebx & 0x000000FF);
        if (edx & 0x00080000) {
            printf("CFLUSH Line Size: %ld\n", (ebx & 0x0000FF00) >> 5);
        }
        if (edx & 0x10000000) {
            printf("Max Logical Processor ID: %ld\n", (ebx & 0x00FF0000) >> 16);
        }
        if (edx & 0x00000200) {
            printf("Local APIC ID: %ld\n", (ebx & 0xFF000000) >> 16);
        }

        printf("CPU Features: ");

        for (int i = 0; i < 32; i++) {
            if ((ecx >> i) & 1) {
                static const char *features[] = {
                    "SSE3",     "PCLMUL",   "DTES64",   "MONITOR",
                    "DS_CPL",   "VMX",      "SMX",      "EST",
                    "TM2",      "SSSE3",    "CID",      "SDBG",
                    "FMA",      "CX16",     "XTPR",     "PDCM",
                    NULL,       "PCID",     "DCA",      "SSE4.1",
                    "SSE4.2",   "X2APIC",   "MOVBE",    "POPCNT",
                    "TSC",      "AES",      "XSAVE",    "OSXSAVE",
                    "AVX",      "F16C",     "RDRAND",   "HYPERVISOR",
                };
                if (!features[i]) continue;

                printf("%s ", features[i]);
            }
        }

        for (int i = 0; i < 32; i++) {
            if ((edx >> i) & 1) {
                static const char *features[] = {
                    "FPU",      "VME",      "DE",       "PSE",
                    "TSC",      "MSR",      "PAE",      "MCE",
                    "CX8",      "APIC",     NULL,       "SEP",
                    "MTRR",     "PGE",      "MCA",      "CMOV",
                    "PAT",      "PSE36",    "PSN",      "CLFLUSH",
                    NULL,       "DS",       "ACPI",     "MMX",
                    "FXSR",     "SSE",      "SSE2",     "SS",
                    "HTT",      "TM",       "IA64",     "PBE",
                };
                if (!features[i]) continue;
                
                printf("%s ", features[i]);
            }
        }

        printf("\n");
    }

    _i686_cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    max_param = eax;

    if (max_param >= 0x80000001) {
        printf("CPU Features (More): ");

        for (int i = 0; i < 32; i++) {
            if ((ecx >> i) & 1) {
                static const char *features[] = {
                    "LAHF_LM",      "CMP_LEGACY",   "SVM",      "EXTAPIC",
                    "CR8_LEGACY",   "ABM_LZCNT",    "SSE4A",    "MISALIGNSSE",
                    "3DNOWPREFETCH","OSVW",         "IBS",      "XOP",
                    "SKINIT",       "WDT",          NULL,       "LWP",
                    "FMA4",         "TCE",          NULL,       "NODEID_MSR",
                    NULL,           "TBM",          "TOPOEXT",  "PERFCTR_CORE",
                    "PERFCTR_NB",   "STREAMPERFMON","DBX",      "PERFTSC",
                    "PCX_L2I",      "MONITORX",     "ADDR_MASK_EXT",    NULL,
                };
                if (!features[i]) continue;

                printf("%s ", features[i]);
            }
        }

        for (int i = 0; i < 32; i++) {
            if ((edx >> i) & 1) {
                static const char *features[] = {
                    NULL,       NULL,       NULL,       NULL,
                    NULL,       NULL,       NULL,       NULL,
                    NULL,       NULL,       "SYSCALL",  "SYSCALL",
                    NULL,       NULL,       NULL,       NULL,
                    NULL,       NULL,       NULL,       "ECC",
                    "NX",       NULL,       "MMXEXT",   NULL,
                    "FXSR",     "FXSR_OPT", "PDPE1GB",  "RDTSCP",
                    NULL,       "LM",       "3DNOWEXT", "3DNOW"
                };
                if (!features[i]) continue;

                printf("%s ", features[i]);
            }
        }

        printf("\n");
    }

    if (max_param >= 0x80000004) {
        uint32_t brand_str[12];
        
        _i686_cpuid(0x80000002, &brand_str[0], &brand_str[1], &brand_str[2], &brand_str[3]);
        _i686_cpuid(0x80000003, &brand_str[4], &brand_str[5], &brand_str[6], &brand_str[7]);
        _i686_cpuid(0x80000004, &brand_str[8], &brand_str[9], &brand_str[10], &brand_str[11]);

        printf("Brand String: %48s\n", (char *)brand_str);
    }

    return 0;
}

static int lspci_handler(struct shell_instance *inst, int argc, char **argv)
{
    /*
    struct pci_device pci_devices[32];
    int pci_count = pci_host_scan(pci_devices, ARRAY_SIZE(pci_devices));

    for (int i = 0; i < pci_count; i++) {
        uint8_t bus = pci_devices[i].bus;
        uint8_t device = pci_devices[i].device;
        uint8_t function = pci_devices[i].function;
        printf("bus %d device %d function %d: vendor 0x%04X, device 0x%04X (%d, %d, %d)\n",
            bus, device, function,
            pci_devices[i].vendor_id, pci_devices[i].device_id,
            pci_devices[i].base_class, pci_devices[i].sub_class, pci_devices[i].interface
        );
        printf("%s -> %s -> %s\n",
            _bus_pci_device_get_class_name(pci_devices[i].base_class),
            _bus_pci_device_get_subclass_name(pci_devices[i].base_class, pci_devices[i].sub_class),
            _bus_pci_device_get_interface_name(pci_devices[i].base_class, pci_devices[i].sub_class, pci_devices[i].interface)
        );
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
        */

    return 0;
}

static void print_dev_tree(struct device *parent_dev, int indent, uint32_t chain) {
    struct device *dev = parent_dev->first_child;

    while (dev) {
        for (int i = 0; i < indent; i++) {
            printf(((chain >> i) & 1) ? "│   " : "    ");
        }
        printf(dev->sibling ? "├───" : "└───");
        printf("%s%d: %s\n", dev->name, dev->id, dev->driver->name);

        if (dev->first_child) {
            print_dev_tree(dev, indent + 1, chain | (dev->sibling ? (1 << indent) : 0));
        }

        dev = dev->sibling;
    }
}

static int lsdev_handler(struct shell_instance *inst, int argc, char **argv)
{
    struct device *dev = get_first_device();
    struct device *last_root_dev = NULL;

    printf("System\n");

    while (dev) {
        if (!dev->parent) {
            if (last_root_dev) {
                printf("├───%s%d: %s\n", last_root_dev->name, last_root_dev->id, last_root_dev->driver->name);
                print_dev_tree(last_root_dev, 1, 1);
            }

            last_root_dev = dev;
        }

        dev = dev->next;
    }

    if (last_root_dev) {
        printf("└───%s%d: %s\n", last_root_dev->name, last_root_dev->id, last_root_dev->driver->name);
        print_dev_tree(last_root_dev, 1, 0);
    }

    return 0;
}

static int acpi_handler(struct shell_instance *inst, int argc, char **argv)
{
    /* List ACPI Info */
    struct acpi_rsdp *rsdp;
    if (uacpi_kernel_get_rsdp((uacpi_phys_addr *)&rsdp)) {
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
    printf("\tOEM ID: %s\n", rsdp->oemid);

    struct acpi_rsdt *rsdt = (struct acpi_rsdt *)rsdp->rsdt_addr;
    printf("\tRSDT: 0x%p\n", (void *)rsdt);
    printf("\t\tCreator ID: %4s\n", (const char *)&rsdt->hdr.creator_id);
    printf("\t\tCreator Revision: 0x%08lX\n", rsdt->hdr.creator_revision);
    printf("\t\tLength: 0x%08lX\n", rsdt->hdr.length);

    if (rsdp->revision >= 2) {
        printf("\tXSDT: 0x%016llX\n", rsdp->xsdt_addr);
    }

    printf("\tTables: \n");
    for (int i = 0; i < (rsdt->hdr.length - sizeof(rsdt->hdr)) / sizeof(uint32_t); i++) {
        struct acpi_sdt_hdr *header = (struct acpi_sdt_hdr *)(rsdt->entries[i]);
        printf("\t- %4s: 0x%p\n", header->signature, (void *)header);
    }

    struct acpi_fadt *fadt;
    if (!uacpi_table_fadt(&fadt)) {
        printf("FADT: 0x%p\n", (void *)fadt);

        printf("\tPreferred PM Profile: %d\n", fadt->preferred_pm_profile);
        printf("\tSCI Interrupt: %d\n", fadt->sci_int);
        printf("\tSMI Command Port: %08lX\n", fadt->smi_cmd);

        printf("\tACPI Enable Port: %04X\n", fadt->acpi_enable);
        printf("\tACPI Disable Port: %04X\n", fadt->acpi_disable);

        printf("\tPM1: evt_len=%d, ctl_len=%d\n", fadt->pm1_evt_len, fadt->pm1_cnt_len);
        printf("\tPM1a: evt_blk=0x%08lX, ctl_blk=0x%08lX\n", fadt->pm1a_evt_blk, fadt->pm1a_cnt_blk);
        printf("\tPM1b: evt_blk=0x%08lX, ctl_blk=0x%08lX\n", fadt->pm1b_evt_blk, fadt->pm1b_cnt_blk);
        
        printf("\tPM2: ctl_blk=0x%08lX, ctl_len=%d\n", fadt->pm2_cnt_blk, fadt->pm2_cnt_len);

        printf("\tPM Timer: block=0x%08lX, length=%d\n", fadt->pm_tmr_blk, fadt->pm_tmr_len);

        printf("\tGPE0: block=0x%08lX, length=%d\n", fadt->gpe0_blk, fadt->gpe0_blk_len);
        printf("\tGPE1: base=%d, block=0x%08lX, length=%d\n", fadt->gpe1_base, fadt->gpe1_blk, fadt->gpe1_blk_len);

        printf("\tCSTATE Control: %02X\n", fadt->cst_cnt);
        printf("\tWorst Latency: c2=%u c3=%u\n", fadt->p_lvl2_lat, fadt->p_lvl3_lat);
        printf("\tCPU Cache Flush: size=%u, stride=%u\n", fadt->flush_size, fadt->flush_stride);
        printf("\tDuty Cycle Setting: offset=%u, width=%u\n", fadt->duty_offset, fadt->duty_width);
        printf("\tRTC Alarm Offset: day=%u, month=%u\n", fadt->day_alrm, fadt->mon_alrm);
        printf("\tRTC Century Offset: %u\n", fadt->century);

        if (fadt->hdr.revision >= 3) {
            printf("\tIA-PC Boot Architecture Flags: %04X\n", fadt->iapc_boot_arch);
        }
    }

    struct uacpi_table table;
    struct acpi_madt *madt;
    if (!uacpi_table_find_by_signature(ACPI_MADT_SIGNATURE, &table)) {
        madt = table.ptr;
        printf("MADT: 0x%p\n", (void *)madt);

        printf("\tLocal APIC Address: 0x%08lX\n", madt->local_interrupt_controller_address);
        printf("\t8259 PIC Installed: %s\n", (madt->flags & 1) ? "true" : "false");

        union {
            struct acpi_entry_hdr header;
            struct acpi_madt_lapic lapic;
            struct acpi_madt_ioapic ioapic;
            struct acpi_madt_interrupt_source_override interrupt_source_override;
            struct acpi_madt_nmi_source nmi_src;
            struct acpi_madt_lapic_nmi lapic_nmi;
            struct acpi_madt_lapic_address_override lapic_address_override;
            struct acpi_madt_x2apic x2apic;
        } *entry = (void *)((uint8_t *)madt + sizeof(*madt));

        while ((ptrdiff_t)entry - (ptrdiff_t)madt < madt->hdr.length) {
            switch (entry->header.type) {
                case ACPI_MADT_ENTRY_TYPE_LAPIC:
                    printf("\tProcessor Local APIC Entry:\n");
                    printf("\t\tACPI Processor ID: 0x%02X\n", entry->lapic.uid);
                    printf("\t\tLAPIC ID: 0x%02X\n", entry->lapic.id);
                    printf("\t\tFlags: 0x%08lX\n", entry->lapic.flags);
                    break;
                case ACPI_MADT_ENTRY_TYPE_IOAPIC:
                    printf("\tI/O APIC Entry:\n");
                    printf("\t\tID: 0x%02X\n", entry->ioapic.id);
                    printf("\t\tAddress: 0x%08lX\n", entry->ioapic.address);
                    printf("\t\tGlobal System Interrupt Base: 0x%08lX\n", entry->ioapic.gsi_base);
                    break;
                case ACPI_MADT_ENTRY_TYPE_INTERRUPT_SOURCE_OVERRIDE:
                    printf("\tInterrupt Source Override Entry:\n");
                    printf("\t\tBus: 0x%02X\n", entry->interrupt_source_override.bus);
                    printf("\t\tIRQ: 0x%02X\n", entry->interrupt_source_override.source);
                    printf("\t\tGlobal System Interrupt: 0x%08lX\n", entry->interrupt_source_override.gsi);
                    printf("\t\tFlags: 0x%04X\n", entry->interrupt_source_override.flags);
                    break;
                case ACPI_MADT_ENTRY_TYPE_NMI_SOURCE:
                    printf("\tNMI Source Entry:\n");
                    break;
                case ACPI_MADT_ENTRY_TYPE_LAPIC_NMI:
                    printf("\tLocal APIC NMI Entry:\n");
                    printf("\t\tACPI Processor ID: 0x%02X\n", entry->lapic_nmi.uid);
                    printf("\t\tFlags: 0x%04X\n", entry->lapic_nmi.flags);
                    printf("\t\tLocal APIC LINT#: 0x%02X\n", entry->lapic_nmi.lint);
                    break;
                case ACPI_MADT_ENTRY_TYPE_LAPIC_ADDRESS_OVERRIDE:
                    printf("\tLocal APIC Address Override Entry:\n");
                    printf("\t\tAddress: 0x%016llX\n", entry->lapic_address_override.address);
                    break;
                default:
                    break;
            }

            entry = (void *)((uint8_t *)entry + entry->header.length);
        }
    }

    struct acpi_hpet *hpet;
    if (!uacpi_table_find_by_signature(ACPI_HPET_SIGNATURE, &table)) {
        hpet = table.ptr;
        printf("HPET: 0x%p\n", (void *)hpet);

        printf("\tBlock ID: %08lX\n", hpet->block_id);
        printf("\tAddress: asp=%u width=%u offset=%u asz=%u address=%016llX\n",
            hpet->address.address_space_id, hpet->address.register_bit_width, hpet->address.register_bit_offset,
            hpet->address.access_size, hpet->address.address
        );
        printf("\tNumber: %u\n", hpet->number);
        printf("\tMinimum Clock Tick: %u\n", hpet->min_clock_tick);
        printf("\tFlags: %02X\n", hpet->flags);
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
    fputs("\x1b[0m", stdout);
    fputs("\x1b[1mbold\x1b[0m\n", stdout);
    fputs("\x1b[2mdim\x1b[0m\n", stdout);
    fputs("\x1b[3mitalic\x1b[0m\n", stdout);
    fputs("\x1b[4munderline\x1b[0m\n", stdout);
    fputs("\x1b[5mblink slow\x1b[0m\n", stdout);
    fputs("\x1b[6mblink fast\x1b[0m\n", stdout);
    fputs("\x1b[7mreversed\x1b[0m\n", stdout);
    fputs("\x1b[9mstrike\x1b[0m\n", stdout);
    fputs("\x1b[53moverline\x1b[0m\n", stdout);
    for (int i = 0; i < 8; i++) {
        printf("\x1b[%dm@", 30 + i);
    }
    for (int i = 0; i < 8; i++) {
        printf("\x1b[%dm@", 90 + i);
    }
    fputs("\x1b[0m\n", stdout);
    for (int i = 0; i < 16; i++) {
        printf("\x1b[38;5;%dm@", i);
    }
    fputs("\x1b[0m\n", stdout);
    for (int i = 16; i < 232; i++) {
        printf("\x1b[38;5;%dm@", i);
        if ((i - 16) % 36 == 35) {
            fputs("\x1b[0m\n", stdout);
        }
    }
    for (int i = 232; i < 256; i++) {
        printf("\x1b[38;5;%dm@", i);
    }
    fputs("\x1b[0m\n", stdout);
    for (int g = 0; g < 8; g++) {
        for (int b = 0; b < 8; b++) {
            for (int r = 0; r < 8; r++) {
                printf("\x1b[38;2;%d;%d;%dm@", r * 255 / 7, g * 255 / 7, b * 255 / 7);
            }
            fputs("\x1b[0m", stdout);
        }
        fputs("\x1b[0m\n", stdout);
    }
    for (int i = 0; i < 8; i++) {
        printf("\x1b[%dm ", 40 + i);
    }
    for (int i = 0; i < 8; i++) {
        printf("\x1b[%dm ", 100 + i);
    }
    fputs("\x1b[0m\n", stdout);
    for (int i = 0; i < 16; i++) {
        printf("\x1b[48;5;%dm ", i);
    }
    fputs("\x1b[0m\n", stdout);
    for (int i = 16; i < 232; i++) {
        printf("\x1b[48;5;%dm ", i);
        if ((i - 16) % 36 == 35) {
            fputs("\x1b[0m\n", stdout);
        }
    }
    for (int i = 232; i < 256; i++) {
        printf("\x1b[48;5;%dm ", i);
    }
    fputs("\x1b[0m\n", stdout);
    for (int g = 0; g < 8; g++) {
        for (int b = 0; b < 8; b++) {
            for (int r = 0; r < 8; r++) {
                printf("\x1b[48;2;%d;%d;%dm ", r * 255 / 7, g * 255 / 7, b * 255 / 7);
            }
            fputs("\x1b[0m", stdout);
        }
        fputs("\x1b[0m\n", stdout);
    }
    for (int l = 7; l >= 0; l--) {
        for (int h = 0; h < 64; h++) {
            uint8_t rgb[3];
            hsl2rgb((float)h / 63.0f, 255, l * 255 / 7 , rgb);
            printf("\x1b[48;2;%d;%d;%dm ", rgb[0], rgb[1], rgb[2]);
        }
        fputs("\x1b[0m\n", stdout);
    }
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

                interrupt_disable();
                framebuffer[ypos * width + xpos] = 0xFFFFFF;
                fbif->invalidate(fbdev, xpos, ypos, xpos, ypos);
                fbif->flush(fbdev);
                fbif->present(fbdev);
                interrupt_enable();
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
    if (argc < 2) {
        fprintf(stderr, "usage: %s addr [count]\n", argv[0]);
        return 1;
    }

    uint32_t addr = strtoul(argv[1], NULL, 16);
    uint32_t count = argc < 3 ? 1 : strtoul(argv[2], NULL, 10);

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
    if (argc < 3) {
        fprintf(stderr, "usage: %s addr value\n", argv[0]);
        return 1;
    }

    uint32_t addr = strtoul(argv[1], NULL, 16);
    uint8_t value = strtol(argv[2], NULL, 16);

    *(uint8_t *)addr = value;
    
    return 0;
}

static int in_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s addr size\n", argv[0]);
        return 1;
    }

    uint16_t addr = strtol(argv[1], NULL, 16);

    switch (*argv[2]) {
        case 'b':
        case 'B':
            printf("%02X\n", io_in8(addr));
            break;
        case 'w':
        case 'W':
            printf("%04X\n", io_in16(addr));
            break;
        case 'd':
        case 'D':
            printf("%08lX\n", io_in32(addr));
            break;
        default:
            return 1;
    }

    return 0;
}

static int out_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (argc < 4) {
        fprintf(stderr, "usage: %s addr size value\n", argv[0]);
        return 1;
    }

    uint16_t addr = strtol(argv[1], NULL, 16);
    uint32_t value = strtoul(argv[3], NULL, 16);

    switch (*argv[2]) {
        case 'b':
        case 'B':
            io_out8(addr, value);
            break;
        case 'w':
        case 'W':
            io_out16(addr, value);
            break;
        case 'd':
        case 'D':
            io_out32(addr, value);
            break;
        default:
            return 1;
    }
    
    return 0;
}

static int readblk_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s device lba\n", argv[0]);
        return 1;
    }

    lba_t lba = strtoull(argv[2], NULL, 16);

    struct device *blkdev = find_device(argv[1]);
    if (!blkdev) {
        fprintf(stderr, "%s: could not find device\n", argv[0]);
        return 1;
    }
    const struct block_interface *blkif = blkdev->driver->get_interface(blkdev, "block");

    uint8_t buf[512];
    blkif->read(blkdev, lba, buf, 1);

    hexdump(buf, sizeof(buf), 0);

    return 0;
}

static int mount_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s device fs_name\n", argv[0]);
        return 1;
    }

    struct device *blkdev = find_device(argv[1]);
    if (!blkdev) {
        fprintf(stderr, "%s: could not find device\n", argv[0]);
        return 1;
    }
    
    return fs_auto_mount(blkdev, argv[2]);
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
    if (argc < 2) {
        fprintf(stderr, "usage: %s fs_name\n", argv[0]);
        return 1;
    }

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
    if (argc < 2) {
        fprintf(stderr, "usage: %s dir_name\n", argv[0]);
        return 1;
    }

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
    if (argc < 2) {
        fprintf(stderr, "usage: %s path\n", argv[0]);
        return 1;
    }

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

static int crc32_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s path\n", argv[0]);
        return 1;
    }

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

    uint8_t buf[512];
    int read_count;
    uint32_t checksum = 0xFFFFFFFF;
    while ((read_count = fread(buf, 1, sizeof(buf), fp)) > 0) {
        checksum = crc32(checksum, buf, read_count);
    }
    printf("%08lX\n", checksum);
    
    fclose(fp);

    return 0;
}

static int dispimg_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s path\n", argv[0]);
        return 1;
    }

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
    } __packed;

    struct bmp_dibheader_core {
        uint32_t header_size;
        uint32_t width;
        uint32_t height;
        uint16_t color_planes;
        uint16_t bpp;
    } __packed;

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
    if (argc < 2) {
        fprintf(stderr, "usage: %s drvnum\n", argv[0]);
        return 1;
    }

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
    int null_start = -1;

    for (int i = 0; i < 256; i++) {
        struct isr_table_entry *entry = _pc_get_isr_table_entry(i);

        if (null_start >= 0 && (entry || i == 255)) {
            printf("Interrupt %d-%d: Unhandled\n", null_start, i - 1);
            null_start = -1;
        }

        if (!entry) {
            if (null_start < 0) {
                null_start = i;
            }
        } else {
            printf("Interrupt %d: \n", i);
            while (entry) {
                if (entry->interrupt_handler) {
                    if (entry->dev) {
                        printf("\ttype=int dev=%s%d handler=%p\n", entry->dev->name, entry->dev->id, (void *)entry->interrupt_handler);
                    } else {
                        printf("\ttype=int dev=none handler=%p\n", (void *)entry->interrupt_handler);
                    }
                } else {
                    printf("\ttype=trap handler=%p\n", (void *)entry->trap_handler);
                }

                entry = entry->next;
            }
        }
    }

    return 0;
}

static int chainload_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s drvnum\n", argv[0]);
        return 1;
    }

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
    char path[PATH_MAX];
    if (argc > 1) {
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
    }

    int ret = font_use(argc > 1 ? path : NULL);
    if (ret) return 1;
    
    struct device *condev = find_device("console0");
    if (!condev) {
        return 0;
    }
    const struct console_interface *conif = condev->driver->get_interface(condev, "console");

    int width, height;
    conif->get_dimension(condev, &width, &height);

    conif->invalidate(condev, 0, 0, width - 1, height - 1);
    conif->flush(condev);
    conif->present(condev);

    return 0;
}

static int jump_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s addr\n", argv[0]);
        return 1;
    }

    uint32_t addr = strtol(argv[1], NULL, 16);
    return ((int (*)(void))addr)();
}

static int boot_handler(struct shell_instance *inst, int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s path\n", argv[0]);
        return 1;
    }

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

    struct elf_file *elf = elf_open(path);
    if (!elf) {
        fprintf(stderr, "%s: failed to open file\n", argv[0]);
        return 1;
    }

    int idx = 0;
    do {
        struct elf_program_header phdr;

        int err = elf_get_current_program_header(elf, &phdr);
        if (err) break;

        printf("phdr #%d:\n", idx);
        printf("\toffset: %08lX\n", phdr.elf32.offset);
        printf("\tvaddr: %08lX\n", phdr.elf32.vaddr);
        printf("\tpaddr: %08lX\n", phdr.elf32.paddr);
        printf("\tfilesz: %08lX\n", phdr.elf32.filesz);
        printf("\tmemsz: %08lX\n", phdr.elf32.memsz);

        elf_load_current_program_header(elf);

        idx++;
    } while (!elf_advance_program_header(elf));
    
    struct device *fbdev = find_device("video0");
    if (!fbdev) {
        fprintf(stderr, "%s: cannot find device\n", argv[0]);
        return 1;
    }

    const struct framebuffer_interface *fbif = fbdev->driver->get_interface(fbdev, "framebuffer");

    struct fb_hw_mode hwmode;

    fbif->get_hw_mode(fbdev, &hwmode);

    cleanup();

    char arg_buffer[1024];
    static const char *memory_model_lut[] = { "direct", "text" } ;

    snprintf(
        arg_buffer,
        sizeof(arg_buffer),
        "--early_fb addr=%p,mm=%s,w=%d,h=%d,p=%d,bpp=%d,r=%d:%d,g=%d:%d,b=%d:%d",
        hwmode.framebuffer,
        memory_model_lut[hwmode.memory_model],
        hwmode.width,
        hwmode.height,
        hwmode.pitch,
        hwmode.bpp,
        hwmode.rmask,
        hwmode.rpos,
        hwmode.gmask,
        hwmode.gpos,
        hwmode.bmask,
        hwmode.bpos
    );

    ((void (*)(char *))elf->ehdr.elf32.entry)(arg_buffer);

    for (;;) {}
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

__noreturn
static int reboot_handler(struct shell_instance *inst, int argc, char **argv)
{
    _i686_pc_reboot();
}

__noreturn
static int poweroff_handler(struct shell_instance *inst, int argc, char **argv)
{
    _i686_pc_poweroff();
}

static int exit_handler(struct shell_instance *inst, int argc, char **argv)
{
    return 0;
}

int shell_handler(struct shell_instance *inst, int argc, char **argv)
{
    struct shell_instance new_inst = {
        .fs = NULL,
        .working_dir = NULL,
        .working_dir_path = { 0, },
    };

    char line_buf[512], elem_buf[512], *newargv[32];
    int result = 0;

    for (;;) {
        if (result) {
            printf("(%d) ", result);
        }
        if (new_inst.fs) {
            printf("%s:%s", new_inst.fs->name, new_inst.working_dir_path);
        }

        shell_readline("> ", line_buf, sizeof(line_buf));

        const char *line_cursor = line_buf;
        char *elem_cursor = elem_buf;
        long elem_buf_len = sizeof(elem_buf);
        int newargc = 0;

        while (newargc < ARRAY_SIZE(newargv)) {
            line_cursor = shell_parse(line_cursor, elem_cursor, elem_buf_len);
            newargv[newargc] = elem_cursor;
            if (!line_cursor) break;

            long elem_len = strnlen(elem_cursor, elem_buf_len);
            elem_cursor += elem_len + 1;
            elem_buf_len -= elem_len + 1;

            newargc++;
        }

        if (newargc < 1) {
            result = 0;
            continue;
        }

        if (strcmp("exit", newargv[0]) == 0) {
            break;
        }

        int command_found = 0;
        for (int i = 0; i < ARRAY_SIZE(commands); i++) {
            if (strcmp(commands[i].name, newargv[0]) == 0) {
                result = commands[i].handler(&new_inst, newargc, newargv);
                command_found = 1;
                break;
            }
        }
    
        if (!command_found) {
            printf("command not found: %s\n", newargv[0]);
            result = 1;
        }
    }

    return 0;
}

void shell_start(void)
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

    interrupt_disable();
}
