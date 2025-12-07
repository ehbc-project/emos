#ifndef __EBOOT_DEBUG_H__
#define __EBOOT_DEBUG_H__

#include <stdint.h>

#include <eboot/compiler.h>

__noreturn
void panic(const char *message);

void stacktrace(const void *base);

void hexdump(const void *data, long len, uint32_t offset);

#endif // __EBOOT_DEBUG_H__
