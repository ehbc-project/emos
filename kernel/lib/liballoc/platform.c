#include "internal.h"

#include <emos/mm.h>
#include <emos/panic.h>

int liballoc_lock(void) {
    return 0;
}

int liballoc_unlock(void) {
    return 0;
}

void *liballoc_alloc(int page_count)
{
    status_t status;
    pfn_t allocated_pfn = (pfn_t)-1;
    vpn_t allocated_vpn = (vpn_t)-1;
    size_t allocated_count = 0;
    
    /* allocate virtual memory pages first */
    status = mm_vma_allocate_page(page_count, &allocated_vpn, VAF_KERNEL);
    if (!CHECK_SUCCESS(status)) goto has_error;

    /* it will allow allocating non-contiguous physical memory frames */
    /* if you want to allocate/map frames/pages for hardware I/O, */
    /* you should use memory management API directly, not this "malloc" API. */
    for (; allocated_count < page_count; allocated_count++) {
        status = mm_vpn_to_pfn(allocated_vpn + allocated_count, NULL);
        if (status != STATUS_PAGE_NOT_PRESENT) goto has_error;

        status = mm_pma_allocate_frame(1, &allocated_pfn, PAF_DEFAULT);
        if (!CHECK_SUCCESS(status)) goto has_error;

        status = mm_map(allocated_pfn, allocated_vpn + allocated_count, 1, PMF_DEFAULT);
        if (!CHECK_SUCCESS(status)) {
            mm_pma_free_frame(allocated_pfn, 1);
            goto has_error;
        }
    }

    return (void *)(allocated_vpn * PAGE_SIZE);

has_error:
    /* allocation failed. rollback changes */
    for (size_t i = 0; i < allocated_count; i++) {
        /* read vpn-pfn mapping to know which frames to free */
        status = mm_vpn_to_pfn(allocated_vpn + i, &allocated_pfn);
        if (!CHECK_SUCCESS(status)) continue;

        /* unmap vpn & free frame*/
        mm_unmap(allocated_vpn + i, 1);
        mm_pma_free_frame(allocated_pfn, 1);
    }

    if (allocated_vpn != (vpn_t)-1) {
        mm_vma_free_page(allocated_vpn, page_count);
    }

    return NULL;
}

int liballoc_free(void *vaddr, int page_count)
{
    status_t status;
    vpn_t vpn = (uintptr_t)vaddr / PAGE_SIZE;
    pfn_t pfn;

    for (size_t i = 0; i < page_count; i++) {
        status = mm_vpn_to_pfn(vpn + i, &pfn);
        if (!CHECK_SUCCESS(status)) {
            panic(STATUS_CONFLICTING_STATE, "tried to free unmapped page");
        }

        mm_unmap(vpn + i, 1);
        mm_pma_free_frame(pfn, 1);
    }

    return 0;
}
