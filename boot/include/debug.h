#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <compiler.h>

__noreturn
void panic(const char *message);

void stacktrace(const void *base);

#endif // __DEBUG_H__
