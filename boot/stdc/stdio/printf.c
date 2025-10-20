#include <stdio.h>

#include <device/driver.h>
#include <interface/char.h>

#undef printf

int printf(const char *fmt, ...)
{
    va_list args;
    
    va_start(args, fmt);
    int ret = vprintf(fmt, args);
    va_end(args);

    return ret;
}