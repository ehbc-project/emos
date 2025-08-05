#include "core/panic.h"

#include <stdio.h>

__attribute__((noreturn))
void panic(const char *message)
{
    fprintf(stderr, "PANIC: %s\r\n", message);
    for (;;) {}
}
