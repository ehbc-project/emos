#ifndef __DEBUG_H__
#define __DEBUG_H__

#include <stdint.h>

#include <compiler.h>

__noreturn
void panic(const char *message);

void stacktrace(const void *base);

void hexdump(const void *data, long len, uint32_t offset);

#endif // __DEBUG_H__
