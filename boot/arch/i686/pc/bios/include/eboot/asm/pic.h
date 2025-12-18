#ifndef __EBOOT_ASM_PIC_H__
#define __EBOOT_ASM_PIC_H__

#include <stdint.h>

void _pc_pic_remap_int(uint8_t master, uint8_t slave);

void _pc_pic_mask_int(int num);
void _pc_pic_unmask_int(int num);

#endif // __EBOOT_ASM_PIC_H__
