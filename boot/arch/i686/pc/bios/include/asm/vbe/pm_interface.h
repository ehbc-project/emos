#ifndef __I686_ASM_VBE_PM_INTERFACE_H__
#define __I686_ASM_VBE_PM_INTERFACE_H__

#include <stdint.h>

#include <asm/bios/video.h>

int _pc_vbe_pmi_set_memory_window(farptr_t pmi_table, int window, uint16_t memory_window);

int _pc_vbe_pmi_set_display_start(farptr_t pmi_table, uint32_t offset);

int _pc_vbe_pmi_set_display_start_vsync(farptr_t pmi_table, uint32_t offset);

int _pc_vbe_pmi_set_palette_data(farptr_t pmi_table, int palette, uint16_t start, uint16_t count, const struct vbe_palette_entry *data);

int _pc_vbe_pmi_set_palette_data_vsync(farptr_t pmi_table, int palette, uint16_t start, uint16_t count, const struct vbe_palette_entry *data);

#endif // __I686_ASM_VBE_PM_INTERFACE_H__
