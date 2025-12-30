#include <emos/mm.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <emos/asm/page.h>
#include <emos/asm/instruction.h>
#include <emos/asm/intrinsics/register.h>
#include <emos/asm/intrinsics/invlpg.h>

#include <emos/macros.h>
#include <emos/panic.h>
#include <emos/log.h>

#define MODULE_NAME "pma"

extern int __end;

#define PBM_FREE            0
#define PBM_ALLOCATED       1
#define PBM_RESERVED        2

#define PBM_GET(idx)        ((pma_bitmap[idx >> 2] >> ((idx & 3) * 2)) & 3)
#define PBM_SET(idx, val) \
    do { \
        pma_bitmap[idx >> 2] &= ~(3 << ((idx & 3) * 2)); \
        pma_bitmap[idx >> 2] |= val << ((idx & 3) * 2); \
    } while (0)

static pfn_t pma_bitmap_pfn_list[64];
static size_t pma_bitmap_pfn_count;

static uint8_t *pma_bitmap;
static size_t pma_bitmap_size;
static size_t pma_frame_desc_count;
static size_t pma_available_frames, pma_free_frames;
static pfn_t pma_base_pfn, pma_limit_pfn;

static union page_table_entry pma_bitmap_page_table[1024] __aligned(PAGE_SIZE);

static status_t pma_allocate_bitmap_frame(pfn_t base_pfn, pfn_t limit_pfn, struct bootinfo_entry_unavailable_frames *ufent)
{
    for (uint32_t i = 0; i < ufent->entry_count; i++) {
        if (ufent->entries[i].pfn < base_pfn) continue;
        if (base_pfn > limit_pfn) break;
        if (pma_bitmap_pfn_count >= ALIGN_DIV(pma_bitmap_size, PAGE_SIZE)) break;
        if (pma_bitmap_pfn_count >= ARRAY_SIZE(pma_bitmap_pfn_list)) break;

        for (pfn_t pfn = base_pfn; pfn < ufent->entries[i].pfn; pfn++) {
            pma_bitmap_pfn_list[pma_bitmap_pfn_count++] = pfn;
            if (pma_bitmap_pfn_count >= ALIGN_DIV(pma_bitmap_size, PAGE_SIZE)) break;
            if (pma_bitmap_pfn_count >= ARRAY_SIZE(pma_bitmap_pfn_list)) break;
        }

        base_pfn = ufent->entries[i].pfn + 1;
    }

    return STATUS_SUCCESS;
}

status_t mm_pma_mark_reserved(pfn_t base_pfn, pfn_t limit_pfn)
{
    if (base_pfn < pma_base_pfn) {
        base_pfn = pma_base_pfn;
    }

    if (limit_pfn > pma_limit_pfn) {
        limit_pfn = pma_limit_pfn;
    }

    for (size_t i = base_pfn - pma_base_pfn; i <= limit_pfn - pma_base_pfn; i++) {
        if (PBM_GET(i) == PBM_RESERVED) continue;

        if (PBM_GET(i) == PBM_FREE) {
            pma_free_frames--;
        }

        pma_available_frames--;

        PBM_SET(i, PBM_RESERVED);
    }

    LOG_DEBUG("marked frame %08lX-%08lX to reserved\n", base_pfn, limit_pfn);

    return STATUS_SUCCESS;
}

status_t mm_pma_unmark_reserved(pfn_t base_pfn, pfn_t limit_pfn)
{
    if (base_pfn < pma_base_pfn) {
        base_pfn = pma_base_pfn;
    }

    if (limit_pfn > pma_limit_pfn) {
        limit_pfn = pma_limit_pfn;
    }

    for (size_t i = base_pfn - pma_base_pfn; i <= limit_pfn - pma_base_pfn; i++) {
        if (PBM_GET(i) != PBM_RESERVED) continue;

        pma_free_frames++;
        pma_available_frames++;

        PBM_SET(i, PBM_FREE);
    }

    LOG_DEBUG("unmarked frame %08lX-%08lX\n", base_pfn, limit_pfn);

    return STATUS_SUCCESS;
}

static status_t pma_bitmap_init(vpn_t pagedir_vpn, struct bootinfo_entry_memory_map *mment, struct bootinfo_entry_unavailable_frames *ufent)
{
    status_t status;
    uintptr_t base_paddr, limit_paddr;
    pfn_t base_pfn, limit_pfn;
    pfn_t bitmap_page_table_pfn;
    union page_dir_entry *pd;
    union page_table_entry *pt;

    /* calculate available area that covers free memory from 0x100000 to 0xFFFFFFFF */
    base_paddr = (uintptr_t)-1;
    limit_paddr = 0;
    for (uint32_t i = 0; i < mment->entry_count; i++) {
        if (mment->entries[i].type != BEMT_FREE) continue;

        if (base_paddr == (uintptr_t)-1) {
            base_paddr = mment->entries[i].base;
        }
        if (limit_paddr < mment->entries[i].base + mment->entries[i].size - 1) {
            limit_paddr = mment->entries[i].base + mment->entries[i].size - 1;
        }
    }

    base_pfn = base_paddr / PAGE_SIZE;
    limit_pfn = limit_paddr / PAGE_SIZE;

    pma_frame_desc_count = limit_pfn - base_pfn + 1;
    pma_available_frames = pma_frame_desc_count;
    pma_free_frames = pma_frame_desc_count;

    /* 2 bits per frame descriptor -> 4 frame descriptors per byte */
    pma_bitmap_size = (pma_frame_desc_count + 3) / 4;

    pma_base_pfn = base_pfn;
    pma_limit_pfn = limit_pfn;

    /* allocate frames */
    pma_bitmap_pfn_count = 0;
    for (uint32_t i = 0; i < mment->entry_count; i++) {
        /* we do not place bitmap to the PRECIOUS conventional memory */
        if (mment->entries[i].base < 0x00100000) continue;
        if (mment->entries[i].type != BEMT_FREE) continue;

        status = pma_allocate_bitmap_frame(mment->entries[i].base / PAGE_SIZE, (mment->entries[i].base + mment->entries[i].size) / PAGE_SIZE, ufent);
        if (!CHECK_SUCCESS(status)) return status;

        if (pma_bitmap_pfn_count >= ALIGN_DIV(pma_bitmap_size, PAGE_SIZE)) break;
    }

    if (pma_bitmap_pfn_count < ALIGN_DIV(pma_bitmap_size, PAGE_SIZE)) {
        for (;;) {}
        return STATUS_INSUFFICIENT_MEMORY;
    }

    pma_bitmap_pfn_count = ALIGN_DIV(pma_bitmap_size, PAGE_SIZE);

    /* get bitmap page table physical frame */
    pt = (void *)(0xFFC00000 + (((uintptr_t)pma_bitmap_page_table & 0xFFC00000) >> 10));
    bitmap_page_table_pfn = pt[((uintptr_t)pma_bitmap_page_table & 0x003FF000) >> 12].base;
    
    memset(pma_bitmap_page_table, 0, sizeof(pma_bitmap_page_table));

    /* map pma bitmap */
    pd = (void *)(pagedir_vpn * PAGE_SIZE);
    pd[0x000C0400 >> 10].dir.base = bitmap_page_table_pfn;
    pd[0x000C0400 >> 10].dir.p = 1;
    pd[0x000C0400 >> 10].dir.r_w = 1;

    pt = (void *)(0xFFC00000 + 0x00301000);
    _i686_invlpg(pt);

    for (size_t i = 0; i < pma_bitmap_pfn_count; i++) {
        pt[i].base = pma_bitmap_pfn_list[i];
        pt[i].p = 1;
        pt[i].r_w = 1;
    }

    /* unmap lower area */
    for (int i = 1; i < 768; i++) {
        if (!pd[i].dir.p) continue;

        pd[i].raw = 0;

        status = mm_pma_unmark_reserved(pd[i].dir.base, pd[i].dir.base);
        if (!CHECK_SUCCESS(status)) return status;
    }

    /* force flush tlb */
    _i686_write_cr3(_i686_read_cr3());

    /* set pma bitmap */
    pma_bitmap = (void *)ALIGN((uintptr_t)&__end, PAGE_SIZE * 1024);

    memset(pma_bitmap, 0, pma_bitmap_size);

    LOG_DEBUG("PMA bitmap initialized to 0x%p. basepfn=0x%08lX limitpfn=0x%08lX\n", pma_bitmap, pma_base_pfn, pma_limit_pfn);

    
    return STATUS_SUCCESS;
}

status_t mm_pma_init(vpn_t pagedir_vpn, struct bootinfo_entry_memory_map *mment, struct bootinfo_entry_unavailable_frames *ufent)
{
    status_t status;

    /* early allocate pma bitmap */
    status = pma_bitmap_init(pagedir_vpn, mment, ufent);
    if (!CHECK_SUCCESS(status)) return status;

    /* mark unavailable frames as reserved */
    for (size_t i = 0; i < pma_bitmap_pfn_count; i++) {
        status = mm_pma_mark_reserved(pma_bitmap_pfn_list[i], pma_bitmap_pfn_list[i]);
        if (!CHECK_SUCCESS(status)) return status;
    }

    for (uint32_t i = 0; i < ufent->entry_count; i++) {
        if (ufent->entries[i].pfn < pma_base_pfn) continue;
        if (ufent->entries[i].pfn > pma_limit_pfn) break;

        status = mm_pma_mark_reserved(ufent->entries[i].pfn, ufent->entries[i].pfn);
        if (!CHECK_SUCCESS(status)) return status;
    }

    return STATUS_SUCCESS;
}

status_t mm_pma_get_available_frame_count(size_t *frame_count)
{
    if (frame_count) *frame_count = pma_available_frames;

    return STATUS_SUCCESS;
}

status_t mm_pma_get_free_frame_count(size_t *frame_count)
{
    if (frame_count) *frame_count = pma_free_frames;

    return STATUS_SUCCESS;
}

status_t mm_pma_allocate_frame(size_t count, pfn_t *pfn, uint32_t flags)
{
    size_t free_count = 0;
    uintptr_t alloc_start_idx = 0;
    
    for (size_t i = 0; i < pma_frame_desc_count; i++) {
        if (pma_base_pfn + i < 0x100) continue;

        if (PBM_GET(i) != PBM_FREE) {
            free_count = 0;
            continue;
        }

        if (free_count == 0) {
            alloc_start_idx = i;
        }

        free_count++;

        if (free_count >= count) {
            break;
        }
    }

    if (free_count < count) {
        return STATUS_INSUFFICIENT_MEMORY;
    }

    for (size_t i = alloc_start_idx; i < alloc_start_idx + count; i++) {
        PBM_SET(i, PBM_ALLOCATED);
        pma_free_frames--;
    }

    if (pfn) *pfn = pma_base_pfn + alloc_start_idx;

    LOG_DEBUG("allocated frame %lu-%lu\n", pma_base_pfn + alloc_start_idx, pma_base_pfn + alloc_start_idx + count - 1);

    return STATUS_SUCCESS;
}

void mm_pma_free_frame(pfn_t pfn, size_t frame_count)
{
    if (pfn < pma_base_pfn || pfn + frame_count > pma_limit_pfn + 1) goto has_error;

    uintptr_t free_start_idx = pfn - pma_base_pfn;

    for (size_t i = free_start_idx; i < free_start_idx + frame_count; i++) {
        if (PBM_GET(i) != PBM_ALLOCATED) goto has_error;

        PBM_SET(i, PBM_FREE);
        pma_free_frames++;
    }

    LOG_DEBUG("freed frame %08lX-%08lX\n", pma_base_pfn + free_start_idx, pma_base_pfn + free_start_idx + frame_count - 1);

has_error:
    panic(STATUS_CONFLICTING_STATE, "failed to free memory frame");
}
