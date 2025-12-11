#include <stdio.h>

#include <string.h>

char *fgets(char *__restrict str, int num, FILE *__restrict stream)
{
    fread(str, 1, num, stream);
    return str;
}
