#ifndef __EBOOT_ASM_PC_GDT_H__
#define __EBOOT_ASM_PC_GDT_H__

#include <eboot/asm/gdt.h>

#include <eboot/status.h>

void _pc_gdt_init(void);

extern struct gdt_entry _pc_gdt[8192];

#endif // __EBOOT_ASM_PC_GDT_H__
