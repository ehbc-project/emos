#ifndef __EMOS_MM_H__
#define __EMOS_MM_H__

#include <stdint.h>
#include <stddef.h>

#include <emos/status.h>
#include <bootemos/bootinfo.h>

#define PAF_DEFAULT         0x00000000
#define PAF_NO_24BIT        0x00000001
#define PAF_NO_32BIT        0x00000002

#define VAF_DEFAULT         0x00000000
#define VAF_KERNEL          0x00000001

#define PMF_DEFAULT          0x00000000
#define PMF_READONLY         0x00000001
#define PMF_USER             0x00000002
#define PMF_NOCACHE          0x00000004
#define PMF_WTCACHE          0x00000008

typedef uintptr_t pfn_t;
typedef uintptr_t vpn_t;

status_t mm_pma_init(vpn_t pagedir_vpn, struct bootinfo_entry_memory_map *mment, struct bootinfo_entry_unavailable_frames *ufent);

status_t mm_pma_get_available_frame_count(size_t *frame_count);
status_t mm_pma_get_free_frame_count(size_t *frame_count);

status_t mm_pma_allocate_frame(size_t frame_count, pfn_t *pfn, uint32_t alloc_flags);
void mm_pma_free_frame(pfn_t pfn, size_t frame_count);


status_t mm_vma_init(vpn_t user_base_vpn, vpn_t user_limit_vpn, vpn_t kernel_base_vpn, vpn_t kernel_limit_vpn);

status_t mm_vma_get_available_kernel_page_count(size_t *page_count);
status_t mm_vma_get_free_kernel_page_count(size_t *page_count);

status_t mm_vma_get_available_user_page_count(size_t *page_count);
status_t mm_vma_get_free_user_page_count(size_t *page_count);

status_t mm_vma_allocate_page(size_t page_count, vpn_t *vpn, uint32_t alloc_flags);
void mm_vma_free_page(vpn_t vpn, size_t page_count);


status_t mm_init(void);

status_t mm_vpn_to_pfn(vpn_t vpn, pfn_t *pfn);
status_t mm_vaddr_to_paddr(void *vaddr, uintptr_t *paddr);

status_t mm_map(pfn_t pfn, vpn_t vpn, size_t page_count, uint32_t flags);
status_t mm_unmap(vpn_t vpn, size_t page_count);

#endif // __EMOS_MM_H__
