#ifndef __MM_MM_H__
#define __MM_MM_H__

#include <stddef.h>

void *mm_allocate(size_t size);
void *mm_allocate_clear(unsigned long count, size_t size);
void mm_free(void *ptr);
void *mm_reallocate(void *ptr, size_t size);

#endif // __MM_MM_H__
