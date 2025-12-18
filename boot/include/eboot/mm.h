#ifndef __EBOOT_MM_H__
#define __EBOOT_MM_H__

#include <stddef.h>
#include <stdint.h>

#include <eboot/status.h>

#define PF_READONLY     0x00000001
#define PF_PRIVILEGED   0x00000002
#define PF_NOCACHE      0x00000004
#define PF_WTCACHE      0x00000008

status_t mm_init(void);

status_t mm_get_memory_usage(size_t *total, size_t *used);

status_t mm_map(uintptr_t paddr, void *vaddr, size_t page_count, uint32_t flags);
status_t mm_unmap(void *vaddr, size_t page_count);

status_t mm_allocate_pages(size_t page_count, void **vaddr);
status_t mm_allocate_pages_to(void *vaddr, size_t page_count);
status_t mm_free_pages(void *vaddr, size_t page_count, size_t *result);

#endif // __EBOOT_MM_H__
