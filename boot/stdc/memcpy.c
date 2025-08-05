#include "string.h"

void *memcpy(void *dest, const void *src, size_t len)
{
    char *dest_c = dest;
    const char *src_c = src;
    while (len--) *dest_c++ = *src_c++;
    return dest;
}