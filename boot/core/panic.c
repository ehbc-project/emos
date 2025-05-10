#include "core/panic.h"

__attribute__((noreturn))
void panic(const char *message)
{
    for (;;) {}
}
