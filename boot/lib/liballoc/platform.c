#include <eboot/mm.h>

int liballoc_lock(void) {
    return 0;
}

int liballoc_unlock(void) {
    return 0;
}

void *liballoc_alloc(int page_count)
{
    void *vaddr;

    if (!CHECK_SUCCESS(mm_allocate_pages(page_count, &vaddr))) {
        return NULL;
    }

    return vaddr;
}

int liballoc_free(void *vaddr, int page_count)
{
    size_t result;

    if (!CHECK_SUCCESS(mm_free_pages(vaddr, page_count, &result))) {
        return 0;
    }

    return result;
}
