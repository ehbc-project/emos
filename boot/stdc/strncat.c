#include <string.h>

#include <stdint.h>

char *strncat(char *dest, const char *src, size_t maxlen)
{
    char *orig_dest = dest;
    while (*dest && maxlen > 0) {
        dest++;
        maxlen--;
    }

    while (*src && maxlen > 0) {
        *dest++ = *src++;
        maxlen--;
    }
    return dest;
}

char *strcat(char *dest, const char *src)
{
    return strncat(dest, src, SIZE_MAX);
}