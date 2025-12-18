
#include <stdio.h>
#include <string.h>
#include <limits.h>

#include <eboot/log.h>
#include <eboot/macros.h>
#include <eboot/status.h>
#include <eboot/shell.h>
#include <eboot/elf.h>
#include <eboot/device.h>
#include <eboot/path.h>
#include <eboot/mm.h>
#include <eboot/interface/video.h>

#define MODULE_NAME "bootemos"

static int bootemos_handler(struct shell_instance *inst, int argc, char **argv)
{
    status_t status;
    struct elf_file *elf = NULL;
    struct elf32_ehdr ehdr;
    struct elf32_phdr phdr;
    size_t program_size = 0;
    void *load_vaddr = NULL;

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

    status = elf_get_header(elf, &ehdr, sizeof(ehdr));
    if (!CHECK_SUCCESS(status)) return 1;

    if (ehdr.type != ET_EXEC) return 1;

    LOG_DEBUG("calculating program offset and size...\n");
    for (int i = 0; i < elf->ehdr32.phnum; i++) {
        status = elf_get_program_header(elf, i, &phdr, sizeof(phdr));
        if (!CHECK_SUCCESS(status)) return 1;

        if (!load_vaddr || (uintptr_t)load_vaddr > phdr.vaddr) {
            load_vaddr = (void *)phdr.vaddr;
        }

        if ((uintptr_t)load_vaddr + program_size < phdr.vaddr + phdr.memsz) {
            program_size = phdr.vaddr + phdr.memsz - (uintptr_t)load_vaddr;
        }
    }
    LOG_DEBUG("offset=0x%p, size=%08lX\n", load_vaddr, program_size);

    status = mm_allocate_pages_to(load_vaddr, ALIGN(program_size, 4096) >> 12);
    if (!CHECK_SUCCESS(status)) return status;
    
    LOG_DEBUG("loading program...\n");
    for (int i = 0; i < elf->ehdr32.phnum; i++) {
        status = elf_get_program_header(elf, i, &phdr, sizeof(phdr));
        if (!CHECK_SUCCESS(status)) return 1;

        if (phdr.type != PT_LOAD) continue;

        status = elf_load_program(elf, i, (void *)phdr.vaddr);
        if (!CHECK_SUCCESS(status)) return 1;
    }
    
    struct device *fbdev;
    status = device_find("video0", &fbdev);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "%s: cannot find device\n", argv[0]);
        return 1;
    }

    const struct video_interface *vidif;
    status = fbdev->driver->get_interface(fbdev, "video", (const void **)&vidif);
    if (!CHECK_SUCCESS(status)) return 1;

    struct video_hw_mode_info hwmode;
    int video_mode;

    status = vidif->get_mode(fbdev, &video_mode);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "%s: cannot get current video mode\n", argv[0]);
        return 1;
    }

    status = vidif->get_hw_mode_info(fbdev, video_mode, &hwmode);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "%s: cannot get video mode hardware info\n", argv[0]);
        return 1;
    }
    
    // cleanup();

    ((void (*)(uint8_t *, int))elf->ehdr32.entry)(hwmode.framebuffer, hwmode.pitch);

    for (;;) {}
}

static struct command bootemos_command = {
    .name = "bootemos",
    .handler = bootemos_handler,
    .help_message = "Boot EMOS",
};

__constructor
static void init()
{
    shell_command_register(&bootemos_command);
}

status_t _start(int argc, char **argv)
{
    return STATUS_SUCCESS;
}

__destructor
static void deinit(void)
{
    shell_command_unregister(&bootemos_command);
}
