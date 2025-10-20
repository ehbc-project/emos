#include <stdio.h>

#undef vfprintf

int vfprintf(FILE *__restrict fp, const char *__restrict fmt, va_list args)
{
    char buf[1024];
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    return fwrite(buf, 1, len, fp);
}
