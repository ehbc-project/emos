#include <eboot/elf.h>

#include <string.h>
#include <stdlib.h>

#include <eboot/macros.h>
#include <eboot/mm.h>

status_t elf_open(const char *path, struct elf_file **elfout)
{
    status_t status;
    struct elf_file *elf = NULL;
    
    elf = malloc(sizeof(struct elf_file));
    if (!elf) {
        status = STATUS_UNKNOWN_ERROR;
        goto has_error;
    }

    elf->fp = fopen(path, "rb");
    if (!elf->fp) {
        status = STATUS_UNKNOWN_ERROR;
        goto has_error;
    }

    fseek(elf->fp, 0, SEEK_SET);
    fread(&elf->ehdr, sizeof(elf->ehdr), 1, elf->fp);

    if (memcmp(elf->ehdr.magic, "\x7F""ELF", sizeof(elf->ehdr.magic)) != 0) {
        status = STATUS_INVALID_SIGNATURE;
        goto has_error;
    }

    elf->phdr_idx = 0;

    if (elfout) *elfout = elf;

    return STATUS_SUCCESS;

has_error:
    if (elf) {
        free(elf->fp);
    }

    return status;
}

void elf_close(struct elf_file *elf)
{
    fclose(elf->fp);
    free(elf);
}

void elf_reset_current_program_header(struct elf_file *elf)
{
    elf->phdr_idx = 0;
}

status_t elf_get_current_program_header(struct elf_file *__restrict elf, struct elf_program_header *__restrict phdr)
{
    if (elf->phdr_idx >= elf->ehdr.elf32.phnum) return STATUS_END_OF_LIST;

    fseek(elf->fp, elf->ehdr.elf32.phoff + elf->phdr_idx * elf->ehdr.elf32.phentsize, SEEK_SET);
    fread(phdr, sizeof(*phdr), 1, elf->fp);

    return STATUS_SUCCESS;
}

status_t elf_load_current_program_header(struct elf_file *__restrict elf)
{
    status_t status;
    struct elf_program_header phdr;

    status = elf_get_current_program_header(elf, &phdr);
    if (!CHECK_SUCCESS(status)) return status;

    if (phdr.type != PHDR_TYPE_LOAD) return STATUS_CONFLICTING_STATE;

    status = mm_allocate_pages_to((void *)phdr.elf32.vaddr, ALIGN(phdr.elf32.memsz, 4096) >> 12);
    if (!CHECK_SUCCESS(status)) return status;

    fseek(elf->fp, phdr.elf32.offset, SEEK_SET);
    fread((void *)phdr.elf32.vaddr, phdr.elf32.filesz, 1, elf->fp);

    if (phdr.elf32.memsz > phdr.elf32.filesz) {
        memset((void *)(phdr.elf32.vaddr + phdr.elf32.filesz), 0, phdr.elf32.memsz - phdr.elf32.filesz);
    }

    return STATUS_SUCCESS;
}

status_t elf_advance_program_header(struct elf_file *elf)
{
    if (elf->phdr_idx >= elf->ehdr.elf32.phnum - 1) return STATUS_END_OF_LIST;

    elf->phdr_idx++;

    return STATUS_SUCCESS;
}
