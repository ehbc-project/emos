#ifndef __EMOS_ASM_PANIC_H__
#define __EMOS_ASM_PANIC_H__

#include <emos/compiler.h>
#include <emos/status.h>

__noreturn
void _pc_panic(status_t status, const char *fmt, ...);

#define panic _pc_panic

#endif // __EMOS_ASM_PANIC_H__
