#ifndef __EBOOT_ASM_PIC_H__
#define __EBOOT_ASM_PIC_H__

#include <stdint.h>

void _pc_remap_pic_int(uint8_t master, uint8_t slave);

#endif // __EBOOT_ASM_PIC_H__
