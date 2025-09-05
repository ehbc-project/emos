#ifndef __I686_PC_BIOS_KEYBOARD_H__
#define __I686_PC_BIOS_KEYBOARD_H__

#include <stdint.h>

/**
 * @brief Read keyboard input, waiting for a key press.
 * @param scancode A pointer to store the scancode of the pressed key.
 * @param ascii A pointer to store the ASCII character of the pressed key.
 */
void _pc_bios_read_keyboard(uint8_t *scancode, char *ascii);

/**
 * @brief Read keyboard input without waiting for a key press.
 * @param scancode A pointer to store the scancode of the pressed key.
 * @param ascii A pointer to store the ASCII character of the pressed key.
 * @return 0 if a key was pressed, otherwise an error code.
 */
int _pc_bios_read_keyboard_nowait(uint8_t *scancode, char *ascii);

/**
 * @brief Read keyboard flags.
 * @return The keyboard flags.
 */
uint16_t _pc_bios_read_keyboard_flags(void);

#endif // __I686_PC_BIOS_KEYBOARD_H__
