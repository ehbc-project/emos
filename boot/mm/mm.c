#include <eboot/mm.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <eboot/asm/page.h>
#include <eboot/asm/instruction.h>
#include <eboot/asm/intrinsics/register.h>

#include <eboot/macros.h>
#include <eboot/log.h>

#define MODULE_NAME "mm"

extern int __end;

static uintptr_t alloc_paddr;
static uintptr_t alloc_vaddr = 0x00400000;
static int invlpg_available;

struct page_dir {
    union page_dir_entry pde[1023];
    union page_table_entry pte;
} __packed;

struct page_dir page_dir __aligned(4096);

struct page_dir *v_page_dir;

static uintptr_t alloc_phys_page(size_t count) {
    uintptr_t new_paddr = alloc_paddr;
    alloc_paddr += count << 12;
    return new_paddr;
}

static uintptr_t alloc_virt_page(size_t count) {
    uintptr_t new_vaddr = alloc_vaddr;
    alloc_vaddr += count << 12;
    return new_vaddr;
}

static void init_page_directory(void)
{
    for (int i = 1; i < 1023; i++) {
        page_dir.pde[i].raw = 0x00000002;
    }

    page_dir.pte.raw = 0x00000003 | ((uintptr_t)&page_dir & 0xFFFFF000);

    uintptr_t new_pt_paddr = alloc_phys_page(1);

    page_dir.pde[0].raw = 0x00000003 | (new_pt_paddr & 0xFFFFF000);

    union page_table_entry *pt = (void *)new_pt_paddr;
    for (int i = 0; i < 1024; i++) {
        pt[i].raw = 0x00000003 | (i << 12);
    }

    v_page_dir = (void *)0xFFFFF000;
}

static void invlpg_test(void)
{
    asm volatile ("invlpg (%0)" : : "r"(0));
}

status_t mm_init(void)
{
    status_t status;

    alloc_paddr = ALIGN((uintptr_t)&__end, 4096);
 
    LOG_DEBUG("initializing page directory...\n");
    init_page_directory();

    LOG_DEBUG("setting up registers...\n");
    _i686_write_cr3((uintptr_t)&page_dir);
    uint32_t cr0 = _i686_read_cr0();
    cr0 |= CR0_PG;
    _i686_write_cr0(cr0);

    LOG_DEBUG("testing whether invlpg available...\n");
    int invlpg_undefined;
    status = _i686_instruction_test(invlpg_test, 3, &invlpg_undefined);
    if (!CHECK_SUCCESS(status)) return status;
    invlpg_available = !invlpg_undefined;

    return STATUS_SUCCESS;
}

status_t mm_get_memory_usage(size_t *total, size_t *used)
{
    if (total) *total = 1048576 * 128;
    if (used) *used = alloc_paddr;

    return STATUS_SUCCESS;
}

static status_t map(uintptr_t paddr, void *vaddr, uint32_t flags)
{
    uintptr_t vaddri = (uintptr_t)vaddr;
    union page_table_entry *pt = (void *)(0xFFC00000 + ((vaddri & 0xFFC00000) >> 10));

    if (!v_page_dir->pde[vaddri >> 22].dir.p) {
        // create a new page table
        uintptr_t new_pt_paddr = alloc_phys_page(1);

        v_page_dir->pde[vaddri >> 22].raw = 0x00000003 | (new_pt_paddr & 0xFFFFF000);

        if (invlpg_available) {
            asm volatile ("invlpg (%0)" : : "r"(pt));
        } else {
            _i686_write_cr3(_i686_read_cr3());
        }
        
        for (int i = 0; i < 1024; i++) {
            pt[i].raw = 0x00000000;
        }
    }

    if (pt[(vaddri & 0x003FF000) >> 12].p) {
        if (!!(flags & PF_READONLY) != !pt[(vaddri & 0x003FF000) >> 12].r_w) {
            return STATUS_CONFLICTING_STATE;
        }
    
        if (!!(flags & PF_PRIVILEGED) != !pt[(vaddri & 0x003FF000) >> 12].u_s) {
            return STATUS_CONFLICTING_STATE;
        }
    
        if (!!(flags & PF_NOCACHE) != !!(pt[(vaddri & 0x003FF000) >> 12].pat && pt[(vaddri & 0x003FF000) >> 12].pcd)) {
            return STATUS_CONFLICTING_STATE;
        }
    
        if (!!(flags & PF_WTCACHE) != !!(pt[(vaddri & 0x003FF000) >> 12].pat && pt[(vaddri & 0x003FF000) >> 12].pwt)) {
            return STATUS_CONFLICTING_STATE;
        }
    }

    pt[(vaddri & 0x003FF000) >> 12].raw = (paddr & 0xFFFFF000);

    if (!(flags & PF_READONLY)) {
        pt[(vaddri & 0x003FF000) >> 12].r_w = 1;
    }

    if (flags & PF_PRIVILEGED) {
        pt[(vaddri & 0x003FF000) >> 12].u_s = 1;
    }

    if ((flags & PF_NOCACHE) || (flags & PF_WTCACHE)) {
        pt[(vaddri & 0x003FF000) >> 12].pat = 1;

        if (flags & PF_NOCACHE) {
            pt[(vaddri & 0x003FF000) >> 12].pcd = 1;
        }

        if (flags & PF_WTCACHE) {
            pt[(vaddri & 0x003FF000) >> 12].pwt = 1;
        }
    }

    pt[(vaddri & 0x003FF000) >> 12].p = 1;

    if (invlpg_available) {
        asm volatile ("invlpg (%0)" : : "r"(pt));
    } else {
        _i686_write_cr3(_i686_read_cr3());
    }

    return STATUS_SUCCESS;
}

status_t mm_map(uintptr_t paddr, void *vaddr, size_t page_count, uint32_t flags)
{
    for (int i = 0; i < page_count; i++) {
        map((paddr & 0xFFFFF000) + i * 4096, (void *)(((uintptr_t)vaddr & 0xFFFFF000) + i * 4096), flags);
    }

    LOG_DEBUG("mapped page %lu-%lu to %lu-%lu\n", paddr >> 12, (paddr + ((page_count - 1) << 12)) >> 12, (uintptr_t)vaddr >> 12, ((uintptr_t)vaddr + ((page_count - 1) << 12)) >> 12);

    return STATUS_SUCCESS;
}

static void unmap(void *vaddr)
{
    uintptr_t vaddri = (uintptr_t)vaddr;
    union page_table_entry *pt = (void *)(0xFFC00000 + ((vaddri & 0xFFC00000) >> 10));

    if (!v_page_dir->pde[vaddri >> 22].dir.p) return;

    if (!pt[(vaddri & 0x003FF000) >> 12].p) return;

    pt[(vaddri & 0x003FF000) >> 12].raw = 0;

    if (invlpg_available) {
        asm volatile ("invlpg (%0)" : : "r"(pt));
    } else {
        _i686_write_cr3(_i686_read_cr3());
    }
}

status_t mm_unmap(void *vaddr, size_t page_count)
{
    LOG_DEBUG("unmapping page %lu-%lu\n", (uintptr_t)vaddr >> 12, ((uintptr_t)vaddr + ((page_count - 1) << 12)) >> 12);

    for (int i = 0; i < page_count; i++) {
        unmap((void *)(((uintptr_t)vaddr & 0xFFFFF000) + i * 4096));
    }

    return STATUS_SUCCESS;
}

status_t mm_allocate_pages(size_t page_count, void **vaddrout)
{
    status_t status;
    
    uintptr_t paddr = alloc_phys_page(page_count) & 0xFFFFF000;
    void *vaddr = (void *)(alloc_virt_page(page_count) & 0xFFFFF000);

    status = mm_map(paddr, vaddr, page_count, 0);
    if (!CHECK_SUCCESS(status)) return status;
    
    if (vaddrout) *vaddrout = (void *)((uintptr_t)vaddr & 0xFFFFF000);

    return STATUS_SUCCESS;
}

status_t mm_allocate_pages_to(void *vaddr, size_t page_count)
{
    status_t status;

    uintptr_t paddr = alloc_phys_page(page_count ) & 0xFFFFF000;
    
    status = mm_map(paddr, (void *)((uintptr_t)vaddr & 0xFFFFF000), page_count, 0);
    if (!CHECK_SUCCESS(status)) return status;
    
    return STATUS_SUCCESS;
}

status_t mm_free_pages(void *vaddr, size_t page_count, size_t *result)
{
    status_t status;
    
    status = mm_unmap((void *)((uintptr_t)vaddr & 0xFFFFF000), page_count);
    if (!CHECK_SUCCESS(status)) return status;

    if (result) *result = page_count;

    return STATUS_SUCCESS;
}
