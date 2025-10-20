#include <string.h>

#include <debug.h>

#undef strcpy

void *__strcpy_chk(void *__restrict dest, const void *__restrict src, size_t destlen)
{
    size_t len = strlen(src) + 1;

    if (destlen < len) {
        panic("__strcpy_chk() failed");
    }

#ifndef __HAVE_BUILTIN_STRCPY
    return strcpy(dest, src);

#else
    return __builtin_strcpy(dest, src);

#endif
}
