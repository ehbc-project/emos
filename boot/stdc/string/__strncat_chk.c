#include <string.h>

#include <debug.h>

#undef strncat

char *__strncat_chk(char *__restrict dest, const char *__restrict src, size_t len, size_t destlen)
{
    if (destlen < len) {
        panic("__strncat_chk() failed");
    }

#ifndef __HAVE_BUILTIN_STRNCAT
    return strncat(dest, src, len);

#else
    return __builtin_strncat(dest, src, len);

#endif
}
