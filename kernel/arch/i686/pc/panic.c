#include <emos/asm/panic.h>

#include <emos/asm/intrinsics/misc.h>

__noreturn
void _pc_panic(status_t status, const char *fmt, ...)
{
    _i686_interrupt_disable();
    for (;;) {
        _i686_halt();
    }
}
