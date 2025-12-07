#ifndef __EBOOT_ASM_FARPTR_H__
#define __EBOOT_ASM_FARPTR_H__

#include <stdint.h>

#include <eboot/compiler.h>

struct farptr {
    uint16_t offset;
    uint16_t segment;
} __packed;

typedef struct farptr farptr_t;

#define FARPTR_TO_VPTR(far_ptr) ((void *)(((uint32_t)(far_ptr).segment << 4) + (far_ptr).offset))

#endif // __EBOOT_ASM_FARPTR_H__