#include <stdio.h>

#include <limits.h>

int sprintf(char* buf, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int ret = vsnprintf(buf, INT_MAX, fmt, args);
    va_end(args);
    return ret;
}