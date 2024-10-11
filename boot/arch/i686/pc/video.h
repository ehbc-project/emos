#ifndef __I686_PC_VIDEO_H__
#define __I686_PC_VIDEO_H__

#include <stdint.h>

int _pc_bios_set_video_mode(uint8_t mode);
void _pc_bios_set_text_cursor_shape(uint16_t shape);
void _pc_bios_set_text_cursor_pos(uint16_t pos);
void _pc_bios_get_text_cursor(uint16_t *shape, uint16_t *pos);
void _pc_bios_scroll_up_text(uint8_t amount, uint8_t left, uint8_t right, uint8_t top, uint8_t bottom, uint8_t attr);
void _pc_bios_scroll_down_text(uint8_t amount, uint8_t left, uint8_t right, uint8_t top, uint8_t bottom, uint8_t attr);
void _pc_bios_read_text_attr_at_cur(uint8_t *ch, uint8_t *attr);
void _pc_bios_write_text_attr_at_cur(uint8_t ch, uint8_t attr, uint16_t count);
void _pc_bios_write_text_at_cur(uint8_t ch, uint16_t count);
void _pc_bios_tty_output(uint8_t ch);
void _pc_bios_write_string(uint8_t mode, uint8_t attr, uint8_t row, uint8_t col, const void *str, uint16_t len);

#endif // __I686_PC_VIDEO_H__
