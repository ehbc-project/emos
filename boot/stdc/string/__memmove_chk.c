#include <string.h>

#include <eboot/debug.h>

#undef memmove

void *__memmove_chk(void *dest, const void *src, size_t len, size_t destlen)
{
    if (destlen < len) {
        panic("__memmove_chk() failed");
    }

#ifndef __HAVE_BUILTIN_MEMMOVE
    return memmove(dest, src, len);

#else
    return __builtin_memmove(dest, src, len);

#endif
}
