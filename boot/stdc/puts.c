#include <stdio.h>

#include <string.h>

int puts(const char *str)
{
    fwrite(str, 1, strlen(str), stdout);
    return 0;
}
