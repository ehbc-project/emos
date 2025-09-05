#include "core/panic.h"

#include <stdio.h>
#include <asm/halt.h>

__attribute__((noreturn))
void panic(const char *message)
{
    fprintf(stderr, "\nPANIC: %s\n", message);
    _i686_pc_halt();
}
