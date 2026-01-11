#ifndef __EMOS_ASM_PC_GDT_H__
#define __EMOS_ASM_PC_GDT_H__

#include <emos/asm/gdt.h>

#define GDT_ENTRY_COUNT 6

#define SEG_SEL_KERNEL_CODE 0x08
#define SEG_SEL_KERNEL_DATA 0x10
#define SEG_SEL_USER_CODE   0x18
#define SEG_SEL_USER_DATA   0x20
#define SEG_SEL_TSS         0x28

void _pc_gdt_init(void);

#endif // __EMOS_ASM_PC_GDT_H__
