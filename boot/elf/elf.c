#include <eboot/elf.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <eboot/macros.h>
#include <eboot/mm.h>

status_t elf_open(const char *path, struct elf_file **elfout)
{
    status_t status;
    struct elf_file *elf = NULL;
    struct elf32_shdr shdr32;
    unsigned int strtab_idx = -1;
    unsigned int symtab_idx = -1;
    
    elf = calloc(1, sizeof(struct elf_file));
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
    fread(&elf->ident, sizeof(elf->ident), 1, elf->fp);

    if (memcmp(elf->ident.magic, ELFMAG, sizeof(elf->ident.magic)) != 0) {
        status = STATUS_INVALID_SIGNATURE;
        goto has_error;
    }
    if (elf->ident.class != ELFCLASS32) {
        status = STATUS_UNSUPPORTED;
        goto has_error;
    }
    if (elf->ident.endianness != ELFDATA2LSB) {
        status = STATUS_UNSUPPORTED;
        goto has_error;
    }
    if (elf->ident.header_version != EV_CURRENT) {
        status = STATUS_UNSUPPORTED;
        goto has_error;
    }

    fseek(elf->fp, 0, SEEK_SET);
    fread(&elf->ehdr32, sizeof(elf->ehdr32), 1, elf->fp);

    if (elf->ehdr32.machine != EM_386) {
        status = STATUS_UNSUPPORTED;
        goto has_error;
    }

    status = elf_get_section_header(elf, elf->ehdr32.shstrndx, &shdr32, sizeof(shdr32));
    if (!CHECK_SUCCESS(status)) goto has_error;
    
    elf->shstrtab = malloc(shdr32.size);
    status = elf_load_section(elf, elf->ehdr32.shstrndx, elf->shstrtab, shdr32.size);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = elf_find_section(elf, ".strtab", &strtab_idx);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = elf_get_section_header(elf, strtab_idx, &shdr32, sizeof(shdr32));
    if (!CHECK_SUCCESS(status)) goto has_error;
    
    elf->strtab = malloc(shdr32.size);
    status = elf_load_section(elf, strtab_idx, elf->strtab, shdr32.size);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = elf_find_section(elf, ".symtab", &symtab_idx);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = elf_get_section_header(elf, symtab_idx, &shdr32, sizeof(shdr32));
    if (!CHECK_SUCCESS(status)) goto has_error;

    elf->symtab32 = malloc(shdr32.size);
    elf->symtab_size = shdr32.size;

    status = elf_load_section(elf, symtab_idx, elf->symtab32, elf->symtab_size);
    if (!CHECK_SUCCESS(status)) goto has_error;

    if (elfout) *elfout = elf;

    return STATUS_SUCCESS;

has_error:
    if (elf && elf->strtab) {
        free(elf->strtab);
    }

    if (elf && elf->shstrtab) {
        free(elf->shstrtab);
    }

    if (elf && elf->fp) {
        fclose(elf->fp);
    }

    if (elf) {
        free(elf);
    }

    return status;
}

void elf_close(struct elf_file *elf)
{
    fclose(elf->fp);
    free(elf);
}

status_t elf_get_header(struct elf_file *elf, void *buf, size_t len)
{
    if (elf->ident.class == ELFCLASS32) {
        memcpy(buf, &elf->ehdr32, MIN(len, sizeof(elf->ehdr32)));
    } else if (elf->ident.class == ELFCLASS64) {
        memcpy(buf, &elf->ehdr64, MIN(len, sizeof(elf->ehdr64)));
    } else {
        return STATUS_UNSUPPORTED;
    }

    return STATUS_SUCCESS;
}

status_t elf_get_program_header(struct elf_file * elf, unsigned int index, void *buf, size_t len)
{
    if (elf->ident.class != ELFCLASS32) return STATUS_UNSUPPORTED;
    
    if (index >= elf->ehdr32.phnum) return STATUS_INVALID_VALUE;

    fseek(elf->fp, elf->ehdr32.phoff + index * elf->ehdr32.phentsize, SEEK_SET);
    fread(buf, MIN(len, elf->ehdr32.phentsize), 1, elf->fp);

    return STATUS_SUCCESS;
}

status_t elf_load_program(struct elf_file *elf, unsigned int index, void *vaddr)
{
    status_t status;
    struct elf32_phdr phdr32;

    if (elf->ident.class != ELFCLASS32) return STATUS_UNSUPPORTED;

    status = elf_get_program_header(elf, index, &phdr32, sizeof(phdr32));
    if (!CHECK_SUCCESS(status)) return status;

    status = mm_allocate_pages_to(
        (uintptr_t)vaddr / PAGE_SIZE,
        ALIGN_DIV((uintptr_t)vaddr % PAGE_SIZE + phdr32.memsz, PAGE_SIZE)
    );
    if (!CHECK_SUCCESS(status)) return status;

    fseek(elf->fp, phdr32.offset, SEEK_SET);
    fread(vaddr, phdr32.filesz, 1, elf->fp);

    if (phdr32.memsz > phdr32.filesz) {
        memset((void *)(phdr32.vaddr + phdr32.filesz), 0, phdr32.memsz - phdr32.filesz);
    }

    return STATUS_SUCCESS;
}

status_t elf_get_section_header(struct elf_file *elf, unsigned int index, void *buf, size_t len)
{
    if (elf->ident.class != ELFCLASS32) return STATUS_UNSUPPORTED;

    if (index >= elf->ehdr32.shnum) return STATUS_INVALID_VALUE;

    fseek(elf->fp, elf->ehdr32.shoff + index * elf->ehdr32.shentsize, SEEK_SET);
    fread(buf, MIN(len, elf->ehdr32.shentsize), 1, elf->fp);

    return STATUS_SUCCESS;
}

status_t elf_get_section_name(struct elf_file *elf, unsigned int index, const char **name)
{
    status_t status;
    struct elf32_shdr shdr32;

    if (elf->ident.class != ELFCLASS32) return STATUS_UNSUPPORTED;

    status = elf_get_section_header(elf, index, &shdr32, sizeof(shdr32));
    if (!CHECK_SUCCESS(status)) return status;

    if (name) *name = elf->shstrtab + shdr32.name;

    return STATUS_SUCCESS;
}

status_t elf_find_section(struct elf_file *elf, const char *name, unsigned int *idx)
{
    const char *section_name;
    status_t status;

    for (int i = 0; i < elf->ehdr32.shnum; i++) {
        status = elf_get_section_name(elf, i, &section_name);
        if (!CHECK_SUCCESS(status)) return status;

        if (strcmp(section_name, name) == 0) {
            if (idx) *idx = i;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_ENTRY_NOT_FOUND;
}

status_t elf_load_section(struct elf_file *elf, unsigned int index, void *buf, size_t len)
{
    status_t status;
    struct elf32_shdr shdr32;

    status = elf_get_section_header(elf, index, &shdr32, sizeof(shdr32));
    if (!CHECK_SUCCESS(status)) return status;

    fseek(elf->fp, shdr32.offset, SEEK_SET);
    fread(buf, MIN(shdr32.size, len), 1, elf->fp);

    return STATUS_SUCCESS;
}

status_t elf_find_symbol(struct elf_file *elf, const char *name, unsigned int *index)
{
    const char *sym_name;

    if (elf->ident.class != ELFCLASS32) return STATUS_UNSUPPORTED;

    for (int i = 0; (i + 1) * sizeof(*elf->symtab32) <= elf->symtab_size; i++) {
        sym_name = elf->strtab + elf->symtab32[i].name;

        if (strcmp(name, sym_name) == 0) {
            if (index) *index = i;
            return STATUS_SUCCESS;
        }
    }

    return STATUS_ENTRY_NOT_FOUND;
}

status_t elf_get_symbol(struct elf_file *elf, unsigned int index, void *buf, size_t len)
{
    if (elf->ident.class != ELFCLASS32) return STATUS_UNSUPPORTED;

    if (index * sizeof(struct elf32_sym) >= elf->symtab_size) {
        return STATUS_INVALID_VALUE;
    }

    memcpy(buf, &elf->symtab32[index], MIN(len, sizeof(struct elf32_sym)));

    return STATUS_SUCCESS;
}

status_t elf_get_symbol_count(struct elf_file *elf, unsigned int *count)
{
    if (elf->ident.class != ELFCLASS32) return STATUS_UNSUPPORTED;

    if (count) *count = elf->symtab_size / sizeof(struct elf32_sym);
    
    return STATUS_SUCCESS;
}
