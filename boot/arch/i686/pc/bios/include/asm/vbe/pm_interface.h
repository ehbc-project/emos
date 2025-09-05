#ifndef __I686_ASM_VBE_PM_INTERFACE_H__
#define __I686_ASM_VBE_PM_INTERFACE_H__

#include <stdint.h>

#include <asm/bios/video.h>

/**
 * @brief Set the VBE protected mode memory window.
 * @param pmi_table The protected mode interface table.
 * @param window The window to set.
 * @param memory_window The memory window.
 * @return 0 on success, otherwise an error code.
 */
int _pc_vbe_pmi_set_memory_window(farptr_t pmi_table, int window, uint16_t memory_window);

/**
 * @brief Set the VBE protected mode display start.
 * @param pmi_table The protected mode interface table.
 * @param offset The display start offset.
 * @return 0 on success, otherwise an error code.
 */
int _pc_vbe_pmi_set_display_start(farptr_t pmi_table, uint32_t offset);

/**
 * @brief Set the VBE protected mode display start with vertical sync.
 * @param pmi_table The protected mode interface table.
 * @param offset The display start offset.
 * @return 0 on success, otherwise an error code.
 */
int _pc_vbe_pmi_set_display_start_vsync(farptr_t pmi_table, uint32_t offset);

/**
 * @brief Set the VBE protected mode palette data.
 * @param pmi_table The protected mode interface table.
 * @param palette The palette to set.
 * @param start The starting index.
 * @param count The number of entries to set.
 * @param data A pointer to the palette data.
 * @return 0 on success, otherwise an error code.
 */
int _pc_vbe_pmi_set_palette_data(farptr_t pmi_table, int palette, uint16_t start, uint16_t count, const struct vbe_palette_entry *data);

/**
 * @brief Set the VBE protected mode palette data with vertical sync.
 * @param pmi_table The protected mode interface table.
 * @param palette The palette to set.
 * @param start The starting index.
 * @param count The number of entries to set.
 * @param data A pointer to the palette data.
 * @return 0 on success, otherwise an error code.
 */
int _pc_vbe_pmi_set_palette_data_vsync(farptr_t pmi_table, int palette, uint16_t start, uint16_t count, const struct vbe_palette_entry *data);

#endif // __I686_ASM_VBE_PM_INTERFACE_H__
