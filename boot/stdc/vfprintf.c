#include <stdio.h>

int vfprintf(FILE *fp, const char *fmt, va_list args)
{
    char buf[1024];
    int len = vsnprintf(buf, sizeof(buf), fmt, args);
    return fwrite(buf, len, 1, fp);
}
