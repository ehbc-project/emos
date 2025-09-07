#include <mm/mm.h>

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern int __end;

static void *alloc_ptr = &__end;

void *mm_allocate(size_t size)
{
    if (size < 1) {
        return NULL;
    }

    uint8_t *ret = (uint8_t *)alloc_ptr + sizeof(size_t);
    ((size_t *)ret)[-1] = size;
    alloc_ptr = ret + size;

    return ret;
}

void *mm_allocate_clear(unsigned long count, size_t size)
{
    void *ret = mm_allocate(count * size);
    memset(ret, 0, count * size);
    return ret;
}

void mm_free(void *ptr)
{
    return;  /* no free */
}

void *mm_reallocate(void *ptr, size_t size)
{
    if (size < 1) {
        mm_free(ptr);
        return NULL;
    }

    if (!ptr) {
        return mm_allocate(size);
    }

    size_t old_size = ((size_t *)ptr)[-1];
    if (size <= old_size) {
        return ptr;
    }

    void *ret = mm_allocate(size);
    for (size_t i = 0; i < old_size; i++) {
        ((uint8_t *)ret)[i] = ((uint8_t *)ptr)[i];
    }
    mm_free(ptr);
    return ret;
}
