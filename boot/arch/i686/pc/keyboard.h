#ifndef __I686_PC_KEYBOARD_H__
#define __I686_PC_KEYBOARD_H__

#include <stdint.h>

void _pc_bios_read_keyboard(uint8_t *scancode, char *ascii);
int _pc_bios_read_keyboard_nowait(uint8_t *scancode, char *ascii);
uint16_t _pc_bios_read_keyboard_flags(void);

#endif // __I686_PC_KEYBOARD_H__
