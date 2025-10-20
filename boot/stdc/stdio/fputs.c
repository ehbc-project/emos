#include <stdio.h>

#include <string.h>

int fputs(const char *__restrict str, FILE *__restrict stream)
{
    fwrite(str, 1, strlen(str), stream);
    return 0;
}
