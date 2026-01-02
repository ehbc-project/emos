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

#define MODULE_NAME "mm"

struct page_dir_recursive *_pc_page_dir;

status_t mm_init(void)
{
    uint32_t cr0;

    LOG_DEBUG("setting up registers...\n");
    cr0 = _i686_read_cr0();
    cr0 |= CR0_PG;
    _i686_write_cr0(cr0);

    _pc_page_dir = (void *)0xFFFFF000;

    return STATUS_SUCCESS;
}

status_t mm_vpn_to_pfn(vpn_t vpn, pfn_t *pfn)
{
    union page_table_entry *pt = (void *)(0xFFC00000 + ((vpn & 0x000FFC00) << 2));

    if (!_pc_page_dir->pde[(vpn & 0x000FFC00) >> 10].dir.p) return STATUS_PAGE_NOT_PRESENT;
    if (!pt[vpn & 0x000003FF].p) return STATUS_PAGE_NOT_PRESENT;

    if (pfn) *pfn = pt[vpn & 0x000003FF].base;

    return STATUS_SUCCESS;
}

status_t mm_vaddr_to_paddr(void *vaddr, uintptr_t *paddr)
{
    status_t status;
    pfn_t pfn;

    status = mm_vpn_to_pfn((uintptr_t)vaddr >> 12, &pfn);
    if (!CHECK_SUCCESS(status)) return status;

    if (paddr) *paddr = pfn * PAGE_SIZE + (((uintptr_t)vaddr) & (PAGE_SIZE - 1));

    return STATUS_SUCCESS;
}

static void invalidate_page(vpn_t vpn)
{
    if (!_pc_invlpg_undefined) {
        _i686_invlpg((void *)(vpn * PAGE_SIZE));
    } else {
        _i686_write_cr3(_i686_read_cr3());
    }
}

static status_t map(pfn_t pfn, vpn_t vpn, uint32_t flags)
{
    status_t status;
    union page_table_entry *pt = (void *)(0xFFC00000 + ((vpn & 0x000FFC00) << 2));
    pfn_t new_pt_pfn;

    if (!_pc_page_dir->pde[(vpn & 0x000FFC00) >> 10].dir.p) {
        // create a new page table
        status = mm_pma_allocate_frame(1, &new_pt_pfn, PAF_DEFAULT);
        if (!CHECK_SUCCESS(status)) return status;

        _pc_page_dir->pde[(vpn & 0x000FFC00) >> 10].raw = 0x00000003 | (new_pt_pfn << 12);

        invalidate_page((uintptr_t)pt >> 12);
        
        for (int i = 0; i < 1024; i++) {
            pt[i].raw = 0x00000000;
        }
    }

    if (pt[vpn & 0x000003FF].p) {
        return STATUS_CONFLICTING_STATE;
    }

    pt[vpn & 0x000003FF].raw = pfn << 12;

    if (!(flags & PMF_READONLY)) {
        pt[vpn & 0x000003FF].r_w = 1;
    }

    if (flags & PMF_USER) {
        pt[vpn & 0x000003FF].u_s = 1;
    }

    if ((flags & PMF_NOCACHE) || (flags & PMF_WTCACHE)) {
        pt[vpn & 0x000003FF].pat = 1;

        if (flags & PMF_NOCACHE) {
            pt[vpn & 0x000003FF].pcd = 1;
        }

        if (flags & PMF_WTCACHE) {
            pt[vpn & 0x000003FF].pwt = 1;
        }
    }

    pt[vpn & 0x000003FF].p = 1;

    invalidate_page(vpn);

    return STATUS_SUCCESS;
}

status_t mm_map(pfn_t pfn, vpn_t vpn, size_t page_count, uint32_t flags)
{
    status_t status;

    for (size_t i = 0; i < page_count; i++) {
        status = map(pfn + i, vpn + i, flags);
        if (!CHECK_SUCCESS(status)) return status;
    }

    // TODO: rollback if failed

    LOG_TRACE("mapped page %lu-%lu to frame %lu-%lu\n", vpn, vpn + page_count - 1, pfn, pfn + page_count - 1);

    return STATUS_SUCCESS;
}

static void unmap(vpn_t vpn)
{
    union page_table_entry *pt = (void *)(0xFFC00000 + ((vpn & 0x000FFC00) << 2));

    if (!_pc_page_dir->pde[(vpn & 0x000FFC00) >> 10].dir.p) return;
    if (!pt[vpn & 0x000003FF].p) return;

    pt[vpn & 0x000003FF].raw = 0;

    invalidate_page(vpn);
}

status_t mm_unmap(vpn_t vpn, size_t page_count)
{
    LOG_TRACE("unmapping page %lu-%lu\n", vpn, vpn + page_count - 1);

    for (size_t i = 0; i < page_count; i++) {
        unmap(vpn + i);
    }

    return STATUS_SUCCESS;
}

status_t mm_allocate_pages(size_t page_count, vpn_t *vpn)
{
    status_t status;
    pfn_t allocated_pfn;
    vpn_t allocated_vpn;

    status = mm_vma_allocate_page(page_count, &allocated_vpn, VAF_DEFAULT);
    
    for (size_t i = 0; i < page_count; i++) {
        status = mm_vpn_to_pfn(allocated_vpn + i, NULL);
        if (!CHECK_SUCCESS(status) && status != STATUS_PAGE_NOT_PRESENT) return status;

        if (status == STATUS_PAGE_NOT_PRESENT) {
            status = mm_pma_allocate_frame(1, &allocated_pfn, PAF_DEFAULT);
            if (!CHECK_SUCCESS(status)) return status;

            status = mm_map(allocated_pfn, allocated_vpn + i, 1, PMF_DEFAULT);
            if (!CHECK_SUCCESS(status)) return status;
        }
    }

    if (vpn) *vpn = allocated_vpn;

    return STATUS_SUCCESS;
}

status_t mm_allocate_pages_to(vpn_t vpn, size_t page_count)
{
    status_t status;
    pfn_t allocated_pfn;

    for (size_t i = 0; i < page_count; i++) {
        status = mm_vpn_to_pfn(vpn + i, NULL);
        if (!CHECK_SUCCESS(status) && status != STATUS_PAGE_NOT_PRESENT) return status;

        if (status == STATUS_PAGE_NOT_PRESENT) {
            status = mm_pma_allocate_frame(1, &allocated_pfn, PAF_DEFAULT);
            if (!CHECK_SUCCESS(status)) return status;

            status = mm_map(allocated_pfn, vpn + i, 1, PMF_DEFAULT);
            if (!CHECK_SUCCESS(status)) return status;
        }
    }
    
    return STATUS_SUCCESS;
}

void mm_free_pages(vpn_t vpn, size_t page_count)
{
    status_t status;
    pfn_t pfn;

    for (size_t i = 0; i < page_count; i++) {
        status = mm_vpn_to_pfn(vpn + i, &pfn);
        if (!CHECK_SUCCESS(status)) continue;
    
        status = mm_unmap(vpn + i, 1);
        if (!CHECK_SUCCESS(status)) continue;
    
        mm_pma_free_frame(pfn, 1);
    }
}
