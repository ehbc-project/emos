#ifndef __EBOOT_ASM_PANIC_H__
#define __EBOOT_ASM_PANIC_H__

#include <eboot/compiler.h>
#include <eboot/status.h>

__noreturn
void _pc_panic(status_t status, const char *fmt, ...);

#define panic _pc_panic

#endif // __EBOOT_ASM_PANIC_H__
