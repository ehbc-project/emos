
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

#include "bootinfo.h"

#define MODULE_NAME "bootemos"

__noreturn
static void jump_kernel(void *entry, struct bootinfo_table_header *btblhdr)
{
    asm volatile (
        "jmp *%1"
        : : "d"(btblhdr), "r"(entry)
    );

    for (;;) {}
}

static int bootemos_handler(struct shell_instance *inst, int argc, char **argv)
{
    status_t status;
    struct elf_file *elf = NULL;
    struct elf32_ehdr ehdr;
    struct elf32_phdr phdr;
    size_t program_size = 0;
    void *load_vaddr = NULL;
    size_t btblentsize;
    size_t btblhdrsize;
    struct bootinfo_table_header *btblhdr;
    struct bootinfo_entry_command_args *entry_command_args;
    struct bootinfo_entry_loader_info *entry_loader_info;
    struct bootinfo_entry_framebuffer *entry_framebuffer;
    uint32_t strtab_cursor;

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

    btblentsize = 0;
    btblhdrsize = 0;

    /* add header size */
    btblhdrsize += sizeof(*btblhdr);

    /* add command args entry size */
    for (int i = 2; i < argc; i++) {
        btblhdrsize += strlen(argv[i]) + 1;
    }
    btblentsize += sizeof(*entry_command_args) + (argc - 2) * sizeof(*entry_command_args->arg_offsets);

    /* add loader info entry size */
    btblhdrsize += sizeof("eboot") + 1;
    btblhdrsize += sizeof("0.0.1") + 1;
    btblhdrsize += sizeof("kms1212") + 1;
    btblentsize += sizeof(*entry_loader_info);

    /* add memory map entry size */
    // TODO: implement

    /* add system disk entry size */
    // TODO: implement

    /* add acpi rsdp entry size */
    // TODO: implement

    /* add framebuffer entry size */
    btblentsize += sizeof(*entry_framebuffer);
    
    /* allocate and fill table */
    btblhdr = malloc(btblhdrsize + btblentsize);
    if (!btblhdr) {
        return 1;
    }
    
    btblhdr->flags = 0;
    btblhdr->version = BTV_CURRENT;
    btblhdr->entry_count = 3;
    btblhdr->entry_start = btblhdrsize;
    strtab_cursor = 0;
    
    entry_command_args = (void *)((uintptr_t)btblhdr + btblhdrsize);
    entry_command_args->header.size = sizeof(*entry_command_args) + (argc - 2) * sizeof(*entry_command_args->arg_offsets);
    entry_command_args->header.type = BET_COMMAND_ARGS;
    entry_command_args->arg_count = argc - 2;

    for (int i = 2; i < argc; i++) {
        strcpy(&btblhdr->strtab[strtab_cursor], argv[i]);
        entry_command_args->arg_offsets[i - 2] = strtab_cursor;
        strtab_cursor += strlen(argv[i]) + 1;
    }

    entry_loader_info = (void *)((uintptr_t)entry_command_args + entry_command_args->header.size);
    entry_loader_info->header.size = sizeof(*entry_loader_info);
    entry_loader_info->header.type = BET_LOADER_INFO;
    entry_loader_info->additional_entry_count = 0;

    strcpy(&btblhdr->strtab[strtab_cursor], "eboot");
    entry_loader_info->name_offset = strtab_cursor;
    strtab_cursor += sizeof("eboot") + 1;
    strcpy(&btblhdr->strtab[strtab_cursor], "0.0.1");
    entry_loader_info->version_offset = strtab_cursor;
    strtab_cursor += sizeof("0.0.1") + 1;
    strcpy(&btblhdr->strtab[strtab_cursor], "kms1212");
    entry_loader_info->author_offset = strtab_cursor;
    strtab_cursor += sizeof("kms1212") + 1;

    entry_framebuffer = (void *)((uintptr_t)entry_loader_info + entry_loader_info->header.size);
    entry_framebuffer->header.size = sizeof(*entry_framebuffer);
    entry_framebuffer->header.type = BET_FRAMEBUFFER;
    entry_framebuffer->framebuffer_addr = (uintptr_t)hwmode.framebuffer;
    entry_framebuffer->width = hwmode.width;
    entry_framebuffer->pitch = hwmode.pitch;
    entry_framebuffer->height = hwmode.height;
    entry_framebuffer->bpp = hwmode.bpp;
    if (hwmode.memory_model == VMM_DIRECT) {
        entry_framebuffer->type = BEFT_DIRECT;

        entry_framebuffer->direct.red_pos = hwmode.rpos;
        entry_framebuffer->direct.red_size = hwmode.rmask;
        entry_framebuffer->direct.green_pos = hwmode.gpos;
        entry_framebuffer->direct.green_size = hwmode.gmask;
        entry_framebuffer->direct.blue_pos = hwmode.bpos;
        entry_framebuffer->direct.blue_size = hwmode.bmask;
    } else {
        entry_framebuffer->type = BEFT_TEXT;
    }

    jump_kernel((void *)elf->ehdr32.entry, btblhdr);
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
