#ifndef __EMOS_MEMORY_H__
#define __EMOS_MEMORY_H__

#include <emos/types.h>
#include <emos/status.h>

status_t emos_memory_map(uintptr_t paddr, void *vaddr, size_t page_count, uint32_t flags);
void emos_memory_unmap(void *vaddr, size_t page_count);

status_t emos_memory_allocate(void **ptr, size_t size, unsigned long align);
void emos_memory_free(void *ptr);

#endif // __EMOS_MEMORY_H__
