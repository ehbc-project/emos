#ifndef __I686_PC_BIOS_VIDEO_H__
#define __I686_PC_BIOS_VIDEO_H__

#include <stdint.h>

#include "asm/farptr.h"

int _pc_bios_set_video_mode(uint8_t mode);

void _pc_bios_set_text_cursor_shape(uint16_t shape);
void _pc_bios_set_text_cursor_pos(uint8_t page, uint8_t row, uint8_t col);
void _pc_bios_get_text_cursor(uint8_t page, uint16_t *shape, uint8_t *row, uint8_t *col);

void _pc_bios_scroll_up_text(uint8_t amount, uint8_t left, uint8_t right, uint8_t top, uint8_t bottom, uint8_t attr);
void _pc_bios_scroll_down_text(uint8_t amount, uint8_t left, uint8_t right, uint8_t top, uint8_t bottom, uint8_t attr);
void _pc_bios_read_text_attr_at_cur(uint8_t *ch, uint8_t *attr);
void _pc_bios_write_text_attr_at_cur(uint8_t ch, uint8_t attr, uint16_t count);
void _pc_bios_write_text_at_cur(uint8_t ch, uint16_t count);
void _pc_bios_tty_output(uint8_t ch);
void _pc_bios_write_string(uint8_t mode, uint8_t attr, uint8_t row, uint8_t col, const void *str, uint16_t len);

void _pc_bios_write_pixel(uint8_t page, uint16_t x, uint16_t y, uint8_t color);
uint8_t _pc_bios_read_pixel(uint8_t page, uint16_t x, uint16_t y);

struct vbe_controller_info {
    char signature[4];
    uint16_t vbe_version;
    farptr_t oem_string;
    uint32_t capabilities;
    farptr_t video_modes;
    uint16_t total_memory;
    uint8_t reserved[492];
} __attribute__((packed));

struct vbe_video_mode_info {
    /* VBE 1.0 */
    uint16_t attributes;
    uint8_t window_a;
    uint8_t window_b;
    uint16_t granularity;
    uint16_t window_size;
    uint16_t segment_a;
    uint16_t segment_b;
    uint32_t win_func_ptr;
    uint16_t pitch;

    /* VBE 1.2 */
    uint16_t width;
    uint16_t height;
    uint8_t w_char;
    uint8_t y_char;
    uint8_t planes;
    uint8_t bpp;
    uint8_t banks;
    uint8_t memory_model;
    uint8_t bank_size;
    uint8_t image_pages;
    uint8_t reserved0;

    uint8_t red_mask;
    uint8_t red_position;
    uint8_t green_mask;
    uint8_t green_position;
    uint8_t blue_mask;
    uint8_t blue_position;
    uint8_t reserved_mask;
    uint8_t reserved_position;
    uint8_t direct_color_attributes;

    /* VBE 2.0 */
    uint32_t framebuffer;
    uint32_t reserved1;
    uint16_t reserved2;

    /* VBE 3.0 */
    uint16_t lin_bytes_per_scan_line;
    uint8_t bnk_num_image_pages;
    uint8_t lin_num_image_pages;
    uint8_t lin_red_mask;
    uint8_t lin_red_position;
    uint8_t lin_green_mask;
    uint8_t lin_green_position;
    uint8_t lin_blue_mask;
    uint8_t lin_blue_position;
    uint8_t lin_reserved_mask;
    uint8_t lin_reserved_position;
    uint32_t max_pixel_clock;
    

    uint8_t reserved3[189];
} __attribute__((packed));

int _pc_bios_get_vbe_controller_info(struct vbe_controller_info *buf);
int _pc_bios_get_vbe_video_mode_info(uint16_t mode, struct vbe_video_mode_info *buf);
int _pc_bios_set_vbe_video_mode(uint16_t mode);

#endif // __I686_PC_BIOS_VIDEO_H__
