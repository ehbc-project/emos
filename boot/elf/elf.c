#include <elf.h>

#include <string.h>
#include <mm/mm.h>

struct elf_file *elf_open(const char *path)
{
    struct elf_file *elf = mm_allocate(sizeof(struct elf_file));
    if (!elf) {
        return NULL;
    }

    elf->fp = fopen(path, "rb");
    if (!elf->fp) {
        mm_free(elf->fp);
        return NULL;
    }

    fseek(elf->fp, 0, SEEK_SET);
    fread(&elf->ehdr, sizeof(elf->ehdr), 1, elf->fp);

    if (memcmp(elf->ehdr.magic, "\x7F""ELF", sizeof(elf->ehdr.magic)) != 0) {
        mm_free(elf->fp);
        return NULL;
    }

    elf->phdr_idx = 0;

    return elf;
}

void elf_close(struct elf_file *elf)
{
    fclose(elf->fp);
    mm_free(elf);
}

void elf_reset_current_program_header(struct elf_file *elf)
{
    elf->phdr_idx = 0;
}

int elf_get_current_program_header(struct elf_file *__restrict elf, struct elf_program_header *__restrict phdr)
{
    if (elf->phdr_idx >= elf->ehdr.elf32.phnum) return 1;

    printf("phoff: %08lX\n", elf->ehdr.elf32.phoff);

    fseek(elf->fp, elf->ehdr.elf32.phoff + elf->phdr_idx * elf->ehdr.elf32.phentsize, SEEK_SET);
    fread(phdr, sizeof(*phdr), 1, elf->fp);

    return 0;
}

int elf_load_current_program_header(struct elf_file *__restrict elf)
{
    struct elf_program_header phdr;

    int err = elf_get_current_program_header(elf, &phdr);
    if (err) return err;

    if (phdr.type != PHDR_TYPE_LOAD) return 1;

    fseek(elf->fp, phdr.elf32.offset, SEEK_SET);
    fread((void *)phdr.elf32.vaddr, phdr.elf32.filesz, 1, elf->fp);

    if (phdr.elf32.memsz > phdr.elf32.filesz) {
        memset((void *)(phdr.elf32.vaddr + phdr.elf32.filesz), 0, phdr.elf32.memsz - phdr.elf32.filesz);
    }

    return 0;
}

int elf_advance_program_header(struct elf_file *elf)
{
    if (elf->phdr_idx >= elf->ehdr.elf32.phnum - 1) return 1;

    elf->phdr_idx++;

    return 0;
}
