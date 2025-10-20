#include <string.h>

#include <debug.h>

#undef stpcpy

char *__stpcpy_chk(char *dest, const char *src, size_t destlen)
{
    size_t len = strlen(src) + 1;

    if (destlen < len) {
        panic("__stpcpy_chk() failed");
    }

#ifndef __HAVE_BUILTIN_STPCPY
    return stpcpy(dest, src);

#else
    return __builtin_stpcpy(dest, src);

#endif
}
