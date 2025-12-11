#include <eboot/module.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include <eboot/macros.h>
#include <eboot/status.h>
#include <eboot/panic.h>
#include <eboot/elf.h>
#include <eboot/path.h>
#include <eboot/mm.h>
#include <eboot/shell.h>

#include "symbol.h"

static status_t resolve_symbol_addr(struct elf_file *elf, unsigned int index, uintptr_t load_vaddr, uintptr_t P, void **addr)
{
    struct elf32_sym sym;
    status_t status;
    char *sym_name = NULL;
    
    status = elf_get_symbol(elf, index, &sym, sizeof(sym));
    if (!CHECK_SUCCESS(status)) goto has_error;

    sym_name = elf->strtab + sym.name;
    fprintf(stderr, "finding symbol %s: ", sym_name);

    if (sym.shndx != 0) {
        if (addr) *addr = (void *)(load_vaddr + sym.value);
        fprintf(stderr, "%p", *addr);
        return STATUS_SUCCESS;
    }

    void *sym_addr;
    status = _module_find_eboot_symbol(sym_name, &sym_addr);
    if (!CHECK_SUCCESS(status)) goto has_error;

    if (addr) *addr = sym_addr;
    fprintf(stderr, "%p", *addr);
    return STATUS_SUCCESS;

has_error:
    if (sym_name) {
        fprintf(stderr, "not found");
    }

    return status;
}

static uint32_t resolve_symbol_value(struct elf_file *elf, unsigned int index)
{
    struct elf32_sym sym;
    status_t status;
    
    status = elf_get_symbol(elf, index, &sym, sizeof(sym));
    if (!CHECK_SUCCESS(status)) return 0;

    return sym.value;
}

static status_t relocate_section(struct elf_file *elf, unsigned int section_idx, unsigned int rel_section_idx, uintptr_t load_vaddr)
{
    status_t status;
    struct elf32_shdr shdr;
    // uintptr_t section_offset;
    struct elf32_rel *rel_section = NULL;
    size_t rel_section_size;
    uint32_t P, A, S, L, B;

    /* get section offset */
    status = elf_get_section_header(elf, section_idx, &shdr, sizeof(shdr));
    if (!CHECK_SUCCESS(status)) goto has_error;

    // section_offset = shdr.offset;

    /* load relocation section */
    status = elf_get_section_header(elf, rel_section_idx, &shdr, sizeof(shdr));
    if (!CHECK_SUCCESS(status)) goto has_error;

    rel_section = malloc(shdr.size);
    rel_section_size = shdr.size;

    status = elf_load_section(elf, rel_section_idx, rel_section, rel_section_size);
    if (!CHECK_SUCCESS(status)) goto has_error;

    /* relocate */
    // fprintf(stddbg, "#,  P,        B,        S,        L,        A,        NEW\n");
    for (int i = 0; (i + 1) * sizeof(struct elf32_rel) <= rel_section_size; i++) {
        B = load_vaddr;
        P = load_vaddr + rel_section[i].offset;
        A = *(uint32_t *)P;

        status = resolve_symbol_addr(elf, rel_section[i].info >> 8, load_vaddr, rel_section[i].offset, (void **)&S);
        if (!CHECK_SUCCESS(status)) return status;

        L = S;

        // fprintf(stddbg, "%02d, %08lX, %08lX, %08lX, %08lX, %08lX, ", (int)(rel_section[i].info & 0xFF), P, S, L, B, A);

        switch (rel_section[i].info & 0xFF) {
            case 0:
                break;
            case 1:
                *(uint32_t *)P = S + A;
                break;
            case 2:
                *(uint32_t *)P = S + A - P;
                break;
            case 3:
                // *(uint32_t *)P = G + A;
                break;
            case 4:
                *(uint32_t *)P = L + A - P;
                break;
            case 5:
                break;
            case 6:
                *(uint32_t *)P = S;
                break;
            case 7:
                *(uint32_t *)P = S;
                break;
            case 8:
                *(uint32_t *)P = B + A;
                break;
            case 9:
                // *(uint32_t *)P = S + A - GOT;
                break;
            case 10:
                // *(uint32_t *)P = GOT + A - P;
                break;
            case 11:
                *(uint32_t *)P = L + A;
                break;
            default:
                break;
        }

        // fprintf(stddbg, "%08lX\n", *(uint32_t *)P);

        fprintf(stderr, " (%p) %08lX -> %08lX\n", (void *)P, A, *(uint32_t *)P);
    }

    free(rel_section);

    return STATUS_SUCCESS;

has_error:
    if (rel_section) {
        free(rel_section);
    }

    return status;
}

struct note_eboot_entry {
    uint16_t type;
    uint16_t key_len;
    uint16_t value_len;
    uint16_t entry_len;
};

static struct module *mod_list_head = NULL;

static status_t run_func_array(struct elf_file *elf, const char *section_name, uintptr_t load_vaddr)
{
    status_t status;
    unsigned int section_idx;
    struct elf32_shdr shdr;
    size_t data_len;

    status = elf_find_section(elf, section_name, &section_idx);
    if (!CHECK_SUCCESS(status)) return status;

    status = elf_get_section_header(elf, section_idx, &shdr, sizeof(shdr));
    if (!CHECK_SUCCESS(status)) return status;

    data_len = shdr.size;

    for (uintptr_t offset = 0; offset < data_len; offset += sizeof(void (*)(void))) {
        void (**func)(void) = (void *)(load_vaddr + shdr.address + offset);

        (*func)();
    }

    return STATUS_SUCCESS;
}

status_t module_load(const char *path, struct module **modout)
{
    status_t status;
    struct elf_file *elf = NULL;
    struct elf32_ehdr ehdr;
    struct elf32_shdr shdr;
    unsigned int note_eboot_section_idx;
    void *note_eboot_section = NULL;
    uintptr_t note_eboot_offset;
    size_t note_eboot_section_len;
    unsigned int section_idx, rel_section_idx;
    char rel_section_name[128];
    uintptr_t load_vaddr = 0xB0000000;
    struct elf32_phdr phdr;
    struct module *mod = NULL;
    char *mod_name = NULL;
    status_t (*entry)(void) = NULL;

    status = elf_open(path, &elf);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = elf_get_header(elf, &ehdr, sizeof(ehdr));
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = elf_find_section(elf, ".note.eboot", &note_eboot_section_idx);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = elf_get_section_header(elf, note_eboot_section_idx, &shdr, sizeof(shdr));
    if (!CHECK_SUCCESS(status)) goto has_error;

    note_eboot_section_len = shdr.size;
    note_eboot_section = malloc(note_eboot_section_len);
    if (!note_eboot_section) {
        status = STATUS_UNKNOWN_ERROR;
        goto has_error;
    }

    status = elf_load_section(elf, note_eboot_section_idx, note_eboot_section, note_eboot_section_len);
    if (!CHECK_SUCCESS(status)) goto has_error;

    for (int i = 0; i < ehdr.phnum; i++) {
        status = elf_get_program_header(elf, i, &phdr, sizeof(phdr));
        if (!CHECK_SUCCESS(status)) goto has_error;

        if (phdr.type != PT_LOAD) continue;

        status = elf_load_program(elf, i, (void *)(load_vaddr + phdr.vaddr));
        if (!CHECK_SUCCESS(status)) goto has_error;
    }

    static const char *section_names[] = {
        ".init_array",
        ".text",
        ".fini_array",
        ".data",
        ".rodata",
    };

    for (int i = 0; i < ARRAY_SIZE(section_names); i++) {
        status = elf_find_section(elf, section_names[i], &section_idx);
        if (status == STATUS_ENTRY_NOT_FOUND) continue;
        if (!CHECK_SUCCESS(status)) goto has_error;

        snprintf(rel_section_name, sizeof(rel_section_name), ".rel%s", section_names[i]);

        status = elf_find_section(elf, rel_section_name, &rel_section_idx);
        if (status == STATUS_ENTRY_NOT_FOUND) continue;
        if (!CHECK_SUCCESS(status)) goto has_error;

        status = relocate_section(elf, section_idx, rel_section_idx, load_vaddr);
        if (!CHECK_SUCCESS(status)) goto has_error;
    }

    mod = malloc(sizeof(*mod));
    if (!mod) {
        status = STATUS_UNKNOWN_ERROR;
        goto has_error;
    }

    note_eboot_offset = 0;
    while (note_eboot_offset < note_eboot_section_len) {
        struct note_eboot_entry *entry = (void *)((uintptr_t)note_eboot_section + note_eboot_offset);
        char *key = (char *)((uintptr_t)entry + sizeof(struct note_eboot_entry));
        void *value = key + entry->key_len + 1;

        if (strncmp("name", key, entry->key_len) == 0) {
            if (entry->type != 1) {
                status = STATUS_INVALID_VALUE;
                goto has_error;
            }

            mod_name = strndup(value, entry->value_len + 1);
            break;
        }

        note_eboot_offset += entry->entry_len;
    }
    if (!mod_name) {
        status = STATUS_INVALID_VALUE;
        goto has_error;
    }

    mod->elf = elf;
    mod->name = mod_name;
    mod->load_vaddr = (void *)load_vaddr;

    free(note_eboot_section);
    note_eboot_section = NULL;

    status = run_func_array(elf, ".init_array", load_vaddr);
    if (!CHECK_SUCCESS(status)) goto has_error;

    entry = (void *)(load_vaddr + ehdr.entry);
    if (entry) {
        status = entry();
        if (!CHECK_SUCCESS(status)) goto has_error;
    }


    if (!mod_list_head) {
        mod_list_head = mod;
    } else {
        struct module *current = mod_list_head;
        for (; current->next; current = current->next) {}

        current->next = mod;
    }

    mod->next = NULL;

    if (modout) *modout = mod;

    return STATUS_SUCCESS;

has_error:
    if (entry) {
        run_func_array(elf, ".fini_array", load_vaddr);
    }

    if (mod_name) {
        free(mod_name);
    }

    if (note_eboot_section) {
        free(note_eboot_section);
    }

    if (mod) {
        free(mod);
    }

    if (elf) {
        elf_close(elf);
    }

    return status;
}

void module_unload(struct module *mod)
{
    if (!mod_list_head) return;

    struct module *prev_mod = NULL;
    for (struct module *current = mod_list_head; current->next; current = current->next) {
        if (current->next == mod) {
            prev_mod = current;
        }
    }
    if (!prev_mod) return;

    prev_mod->next = mod->next;

    run_func_array(mod->elf, ".fini_array", (uintptr_t)mod->load_vaddr);

    free(mod->name);
    free(mod);
}

struct module *module_get_first_mod(void)
{
    return mod_list_head;
}

status_t module_find(const char *name, struct module **mod);

