#include <stdio.h>

#include <debug.h>

#undef snprintf

int __snprintf_chk(char *str, size_t maxlen, int flag, size_t slen, const char *fmt, ...)
{
    if (slen < maxlen) {
        panic("__snprintf_chk() failed");
    }

    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(str, maxlen, fmt, args);
    va_end(args);

    return len;
}
