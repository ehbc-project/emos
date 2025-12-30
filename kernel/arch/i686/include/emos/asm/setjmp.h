#ifndef __EMOS_ASM_SETJMP_H__
#define __EMOS_ASM_SETJMP_H__

#include <stdint.h>

struct jmp_buf {
    uint32_t addr;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t eax;
};

typedef struct jmp_buf jmp_buf[1];

#endif // __EMOS_ASM_SETJMP_H__
