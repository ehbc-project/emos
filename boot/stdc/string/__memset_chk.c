#include <string.h>

#include <eboot/debug.h>

#undef memset

void *__memset_chk(void *dest, int c, size_t len, size_t destlen)
{
    if (destlen < len) {
        panic("__memset_chk() failed");
    }

#ifndef __HAVE_BUILTIN_MEMSET
    return memset(dest, c, len);

#else
    return __builtin_memset(dest, c, len);

#endif
}
