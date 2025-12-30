#ifndef __EBOOT_DEBUG_H__
#define __EBOOT_DEBUG_H__

#include <stdint.h>
#include <stdio.h>

#include <eboot/compiler.h>
#include <eboot/status.h>

void stacktrace(const void *base);

void hexdump(FILE *fp, const void *data, long len, uint32_t offset);

#endif // __EBOOT_DEBUG_H__
