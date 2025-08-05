#include <stdio.h>

#include <stdarg.h>

int fprintf(FILE *fp, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    int len = vfprintf(fp, fmt, args);
    va_end(args);

    return len;
}