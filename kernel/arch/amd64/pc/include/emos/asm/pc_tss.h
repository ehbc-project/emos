#ifndef __EMOS_ASM_PC_TSS_H__
#define __EMOS_ASM_PC_TSS_H__

#include <stdint.h>

#include <emos/asm/tss.h>

void _pc_tss_init(void);
void _pc_tss_set_stack(uintptr_t kstack);

#endif // __EMOS_ASM_PC_TSS_H__
