#ifndef __I686_PC_BIOS_VIDEO_H__
#define __I686_PC_BIOS_VIDEO_H__

#include <stdint.h>

#include <asm/farptr.h>
#include <device/video/vbe.h>

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

struct vbe_pm_interface {
    uint16_t set_window;
    uint16_t set_display_start;
    uint16_t set_primary_palette_data;
    uint16_t port_mem_locations;  /* refer to VBE 3.0 spec page 57 */
} __attribute__((packed));

struct edid_chroma_info {
    uint8_t green_red_xy;
    uint8_t white_blue_xy;
    uint8_t red_y;
    uint8_t red_x;
    uint8_t green_y;
    uint8_t green_x;
    uint8_t blue_y;
    uint8_t blue_x;
    uint8_t white_y;
    uint8_t white_x;
} __attribute__((packed));

struct edid_detailed_timings {
    uint8_t hori_freq;
    uint8_t vert_freq;
    uint8_t hactive;
    uint8_t hblank;
    uint8_t hactive_hblank;
    uint8_t vactive;
    uint8_t vblank;
    uint8_t vactive_vblank;
    uint8_t hsync_offset;
    uint8_t hsync_width;
    uint8_t vsyncoffs_vsyncw;
    uint8_t vhsyncoffs_width;
    uint8_t himage_size_mm;
    uint8_t vimage_size_mm;
    uint8_t himage_vimage_size;
    uint8_t hborder;
    uint8_t vborder;
    uint8_t display_type;
} __attribute__((packed));

struct edid {
    uint8_t padding[8];
    uint16_t manufacturer_id;
    uint16_t edid_id;
    uint32_t serial_number;
    uint8_t manufactured_week;
    uint8_t manufactured_year;
    uint8_t edid_version;
    uint8_t edid_revision;
    uint8_t video_input_type;
    uint8_t max_phys_width;
    uint8_t max_phys_height;
    uint8_t gamma_factor;
    uint8_t dpms_flags;
    struct edid_chroma_info chroma_info;
    uint8_t established_timings[2];
    uint8_t reserved_timing;
    uint16_t standard_timings[8];
    struct edid_detailed_timings detailed_timings[4];
    uint8_t unused;
    uint8_t checksum;
} __attribute__((packed));

int _pc_bios_get_vbe_controller_info(struct vbe_controller_info *buf);
int _pc_bios_get_vbe_video_mode_info(uint16_t mode, struct vbe_video_mode_info *buf);
int _pc_bios_set_vbe_video_mode(uint16_t mode);
int _pc_bios_set_vbe_display_start(uint16_t x, uint16_t y);
int _pc_bios_set_vbe_display_start_at_retrace(uint16_t x, uint16_t y);
int _pc_bios_schedule_vbe_display_start(uint32_t fboffset);
int _pc_bios_schedule_vbe_display_start_at_retrace(uint32_t fboffset);
int _pc_bios_get_vbe_pm_interface(const struct vbe_pm_interface **ptr);
int _pc_bios_get_vbe_edid(uint16_t ctrlr_unit, uint16_t edid_block, struct edid *buf);

#endif // __I686_PC_BIOS_VIDEO_H__
