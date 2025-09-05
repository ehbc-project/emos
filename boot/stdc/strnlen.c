#include <string.h>

#include <stdint.h>

size_t strnlen(const char *str, size_t maxlen)
{
    size_t s = 0;
    for (; *str++ && s <= maxlen; s++);
    return s;
}

size_t strlen(const char *str)
{
    return strnlen(str, SIZE_MAX);
}