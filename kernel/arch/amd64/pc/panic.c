#include <emos/asm/panic.h>

#include <stdio.h>
#include <stdarg.h>

#include <emos/asm/io.h>
#include <emos/asm/intrinsics/misc.h>

static int panic_out(void *, char ch)
{
    if (!ch) return 1;

    io_out8(0x00E9, ch);

    return 0;
}

__noreturn
void _pc_panic(status_t status, const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vcprintf(panic_out, NULL, fmt, args);
    va_end(args);

    _i686_interrupt_disable();
    for (;;) {
        _i686_halt();
    }
}
