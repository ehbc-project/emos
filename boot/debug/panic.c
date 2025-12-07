#include <eboot/debug.h>

#include <stdio.h>

#include <eboot/asm/intrinsics/misc.h>

__noreturn
void panic(const char *message)
{
    fprintf(stderr, "\nPANIC: %s\n", message);
    
    for (;;) {
        _i686_halt();
    }
}
