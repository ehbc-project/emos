#include <debug.h>

#include <stdio.h>
#include <asm/halt.h>

__noreturn
void panic(const char *message)
{
    fprintf(stderr, "\nPANIC: %s\n", message);
    _i686_pc_halt();
}
