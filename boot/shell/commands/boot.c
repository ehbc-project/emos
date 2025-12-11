#include <eboot/shell.h>

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <eboot/status.h>
#include <eboot/elf.h>
#include <eboot/device.h>
#include <eboot/path.h>
#include <eboot/interface/framebuffer.h>

static int boot_handler(struct shell_instance *inst, int argc, char **argv)
{
    status_t status;
    struct elf_file *elf = NULL;
    struct elf32_phdr phdr;

    if (argc < 2) {
        fprintf(stderr, "usage: %s path\n", argv[0]);
        return 1;
    }

    char path[PATH_MAX];
    if (path_is_absolute(argv[1])) {
        strncpy(path, argv[1], sizeof(path) - 1);
    } else {
        strncpy(path, inst->working_dir_path, sizeof(path) - 1);
        path_join(path, sizeof(path), argv[1]);

        if (!inst->fs) {
            fprintf(stderr, "%s: filesystem not selected\n", argv[0]);
            return 1;
        }
    }

    status = elf_open(path, &elf);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "%s: failed to open file\n", argv[0]);
        return 1;
    }

    for (int i = 0; i < elf->ehdr32.phnum; i++) {
        status = elf_get_program_header(elf, i, &phdr, sizeof(phdr));
        if (!CHECK_SUCCESS(status)) return 1;

        printf("phdr #%d:\n", i);
        printf("\toffset: %08lX\n", phdr.offset);
        printf("\tvaddr: %08lX\n", phdr.vaddr);
        printf("\tpaddr: %08lX\n", phdr.paddr);
        printf("\tfilesz: %08lX\n", phdr.filesz);
        printf("\tmemsz: %08lX\n", phdr.memsz);

        elf_load_program(elf, i, (void *)phdr.vaddr);
    }
    
    struct device *fbdev;
    status = device_find("video0", &fbdev);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "%s: cannot find device\n", argv[0]);
        return 1;
    }

    const struct framebuffer_interface *fbif;
    status = fbdev->driver->get_interface(fbdev, "framebuffer", (const void **)&fbif);
    if (!CHECK_SUCCESS(status)) return 1;

    struct fb_hw_mode hwmode;

    fbif->get_hw_mode(fbdev, &hwmode);

    // cleanup();

    char arg_buffer[1024];
    static const char *memory_model_lut[] = { "direct", "text" };

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

    ((void (*)(char *))elf->ehdr32.entry)(arg_buffer);

    for (;;) {}
}

static struct command boot_command = {
    .name = "boot",
    .handler = boot_handler,
    .help_message = "Boot from file",
};

static void boot_command_init(void)
{
    shell_command_register(&boot_command);
}

SHELL_COMMAND(boot, boot_command_init)
