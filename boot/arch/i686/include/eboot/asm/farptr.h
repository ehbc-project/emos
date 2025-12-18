#ifndef __EBOOT_ASM_FARPTR_H__
#define __EBOOT_ASM_FARPTR_H__

#include <stdint.h>

#include <eboot/compiler.h>

struct farptr16 {
    uint16_t offset;
    uint16_t segment;
} __packed;

typedef struct farptr16 farptr16_t;

#define FARPTR16_TO_VPTR(far_ptr) ((void *)(((uintptr_t)(far_ptr).segment << 4) + (far_ptr).offset))

struct farptr32 {
    uint32_t offset;
    uint16_t segment;
} __packed;

typedef struct farptr32 farptr32_t;

#endif // __EBOOT_ASM_FARPTR_H__