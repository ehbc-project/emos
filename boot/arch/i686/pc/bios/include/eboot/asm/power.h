#ifndef __EBOOT_ASM_POWEROFF_H__
#define __EBOOT_ASM_POWEROFF_H__

#include <eboot/compiler.h>

__noreturn
void _pc_poweroff();

__noreturn
void _pc_reboot();

#define poweroff _pc_poweroff
#define reboot _pc_reboot

#endif // __EBOOT_ASM_POWEROFF_H__
