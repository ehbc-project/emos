#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <emos/asm/gdt.h>
#include <emos/asm/pc_gdt.h>
#include <emos/asm/io.h>
#include <emos/asm/page.h>
#include <emos/asm/instruction.h>

#include <emos/compiler.h>
#include <emos/boot/bootinfo.h>
#include <emos/mm.h>
#include <emos/status.h>
#include <emos/macros.h>
#include <emos/panic.h>
#include <emos/log.h>

#define MODULE_NAME "init"

#define SKIP_INVLPG_CHECK
#define SKIP_RDTSC_CHECK

int _pc_invlpg_undefined;
int _pc_rdtsc_undefined;

static void invlpg_test(void)
{
    asm volatile ("invlpg (%0)" : : "r"(0));
}

static void rdtsc_test(void)
{
    uint32_t low, high;
    asm volatile ("rdtsc" : "=a"(low), "=d"(high));
}

struct bootinfo_table_header *_pc_bootinfo_table;

static int early_print_char(void *, char ch)
{
    switch (ch) {
        case '\0':
        case '\r':
            return 1;
        default:
            io_out8(0x00E9, ch);
            return 0;
    }
}

void _pc_init(struct bootinfo_table_header *btblhdr)
{
    status_t status;
    struct bootinfo_entry_header *enthdr = NULL;
    struct bootinfo_entry_memory_map *mment = NULL;
    struct bootinfo_entry_unavailable_frames *ufent = NULL;
    struct bootinfo_entry_pagetable_vpn *pvent = NULL;
    struct bootinfo_table_header *newbtblhdr = NULL;

    log_early_init(early_print_char, NULL);

    LOG_DEBUG("Starting kernel...\n");

    enthdr = (void *)((uintptr_t)btblhdr + btblhdr->header_size);
    for (int i = 0; i < btblhdr->entry_count; i++) {
        switch (enthdr->type) {
            case BET_MEMORY_MAP:
                mment = (void *)((uintptr_t)enthdr + enthdr->header_size);
                break;
            case BET_UNAVAILABLE_FRAMES:
                ufent = (void *)((uintptr_t)enthdr + enthdr->header_size);
                break;
            case BET_PAGETABLE_VPN:
                pvent = (void *)((uintptr_t)enthdr + enthdr->header_size);
                break;
            default:
                break;
        }

        enthdr = (void *)((uintptr_t)enthdr + enthdr->size);
    }

    if (!mment || !ufent || !pvent) {
        panic(STATUS_ENTRY_NOT_FOUND, "required entry not found");
    }

    LOG_DEBUG("testing whether invlpg available...\n");
#ifdef SKIP_INVLPG_CHECK
    _pc_invlpg_undefined = 0;

#else
    status = _pc_instruction_test(invlpg_test, 3, &_pc_invlpg_undefined);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to test instruction invlpg");
    }

#endif

    LOG_DEBUG("testing whether rdtsc available...\n");
#ifdef SKIP_RDTSC_CHECK
    _pc_rdtsc_undefined = 0;

#else
    status = _pc_instruction_test(rdtsc_test, 2, &_pc_rdtsc_undefined);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to test instruction rdtsc");
    }

#endif

    LOG_DEBUG("initializing GDT...\n");
    _pc_gdt_init();

    LOG_DEBUG("initializing physical memory allocator...\n");
    status = mm_pma_init(pvent->vpn, mment, ufent);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to initialize physical memory allocator");
    }

    LOG_DEBUG("initializing virtual memory allocator...\n");
    status = mm_vma_init(0x00000100, 0x000BFFFF, 0x000C1000, 0x000FEFFF);
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to initialize virtual memory allocator");
    }

    LOG_DEBUG("initializing memory management...\n");
    status = mm_init();
    if (!CHECK_SUCCESS(status)) {
        panic(status, "failed to initialize memory management");
    }

    LOG_DEBUG("relocating bootinfo table...\n");
    newbtblhdr = malloc(btblhdr->size);
    if (!newbtblhdr) {
        panic(status, "cannot allocate memory for bootinfo table");
    }
    
    memcpy(newbtblhdr, btblhdr, btblhdr->size);
    
    _pc_bootinfo_table = newbtblhdr;

    LOG_DEBUG("%p %p %p %08lX\n", (void *)_pc_bootinfo_table, (void *)btblhdr, (void *)enthdr, btblhdr->size);
}
