#ifndef __I686_PC_BIOS_VIDEO_H__
#define __I686_PC_BIOS_VIDEO_H__

#include <stdint.h>

#include <asm/farptr.h>

/**
 * @brief Set the video mode.
 * @param mode The video mode to set.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_set_video_mode(uint8_t mode);

/**
 * @brief Set the text cursor shape.
 * @param shape The cursor shape.
 */
void _pc_bios_set_text_cursor_shape(uint16_t shape);

/**
 * @brief Set the text cursor position.
 * @param page The video page.
 * @param row The row.
 * @param col The column.
 */
void _pc_bios_set_text_cursor_pos(uint8_t page, uint8_t row, uint8_t col);

/**
 * @brief Get the text cursor position and shape.
 * @param page The video page.
 * @param shape A pointer to store the cursor shape.
 * @param row A pointer to store the row.
 * @param col A pointer to store the column.
 */
void _pc_bios_get_text_cursor(uint8_t page, uint16_t *shape, uint8_t *row, uint8_t *col);

/**
 * @brief Scroll up text.
 * @param amount The number of lines to scroll.
 * @param left The left column.
 * @param right The right column.
 * @param top The top row.
 * @param bottom The bottom row.
 * @param attr The attribute for the new lines.
 */
void _pc_bios_scroll_up_text(uint8_t amount, uint8_t left, uint8_t right, uint8_t top, uint8_t bottom, uint8_t attr);

/**
 * @brief Scroll down text.
 * @param amount The number of lines to scroll.
 * @param left The left column.
 * @param right The right column.
 * @param top The top row.
 * @param bottom The bottom row.
 * @param attr The attribute for the new lines.
 */
void _pc_bios_scroll_down_text(uint8_t amount, uint8_t left, uint8_t right, uint8_t top, uint8_t bottom, uint8_t attr);

/**
 * @brief Read the character and attribute at the current cursor position.
 * @param ch A pointer to store the character.
 * @param attr A pointer to store the attribute.
 */
void _pc_bios_read_text_attr_at_cur(uint8_t *ch, uint8_t *attr);

/**
 * @brief Write a character and attribute at the current cursor position.
 * @param ch The character to write.
 * @param attr The attribute to use.
 * @param count The number of times to write the character.
 */
void _pc_bios_write_text_attr_at_cur(uint8_t ch, uint8_t attr, uint16_t count);

/**
 * @brief Write a character at the current cursor position.
 * @param ch The character to write.
 * @param count The number of times to write the character.
 */
void _pc_bios_write_text_at_cur(uint8_t ch, uint16_t count);

/**
 * @brief TTY output.
 * @param ch The character to output.
 */
void _pc_bios_tty_output(uint8_t ch);

/**
 * @brief Write a string to the screen.
 * @param mode The write mode.
 * @param attr The attribute to use.
 * @param row The row to write to.
 * @param col The column to write to.
 * @param str A pointer to the string.
 * @param len The length of the string.
 */
void _pc_bios_write_string(uint8_t mode, uint8_t attr, uint8_t row, uint8_t col, const void *str, uint16_t len);

/**
 * @brief Write a pixel to the screen.
 * @param page The video page.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @param color The color of the pixel.
 */
void _pc_bios_write_pixel(uint8_t page, uint16_t x, uint16_t y, uint8_t color);

/**
 * @brief Read a pixel from the screen.
 * @param page The video page.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return The color of the pixel.
 */
uint8_t _pc_bios_read_pixel(uint8_t page, uint16_t x, uint16_t y);

#define VBE_CTRL_SIGNATURE_VBE1 "VESA"
#define VBE_CTRL_SIGNATURE_VBE2 "VBE2"

struct vbe_palette_entry {
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved;
} __attribute__((packed));

struct vbe_crtc_info_block {
    uint16_t htotal;
    uint16_t hsync_start;
    uint16_t hsync_end;
    uint16_t vtotal;
    uint16_t vsync_start;
    uint16_t vsync_end;
    uint8_t flags;
    uint32_t pixel_clock;
    uint16_t refresh_rate;
    uint8_t reserved[40];
} __attribute__((packed));

struct vbe_pm_interface {
    uint16_t set_window;
    uint16_t set_display_start;
    uint16_t set_primary_palette_data;
    uint16_t port_mem_locations;  /* refer to VBE 3.0 spec page 57 */
    uint8_t additional_data[];
} __attribute__((packed));

struct vbe_controller_info {
    /* VBE 1.0 */
    char signature[4];
    uint16_t vbe_version;
    farptr_t oem_string;
    uint32_t capabilities;
    farptr_t video_modes;
    uint16_t total_memory;

    /* VBE 2.0 */
    uint16_t oem_software_rev;
    farptr_t oem_vendor_name_ptr;
    farptr_t oem_product_name_ptr;
    farptr_t oem_product_rev_ptr;

    uint8_t reserved[222];

    uint8_t oem_data[256];
    
} __attribute__((packed));

enum vbe_memory_model {
    VBEMM_TEXT = 0,
    VBEMM_CGA,
    VBEMM_HERCULES,
    VBEMM_PLANAR,
    VBEMM_PACKED,
    VBEMM_NON_CHAIN,
    VBEMM_DIRECT,
    VBEMM_YUV,
};

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

/**
 * @brief Get VBE controller information.
 * @param buf A pointer to the buffer to store the information.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_get_vbe_controller_info(struct vbe_controller_info *buf);

/**
 * @brief Get VBE video mode information.
 * @param mode The video mode.
 * @param buf A pointer to the buffer to store the information.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_get_vbe_video_mode_info(uint16_t mode, struct vbe_video_mode_info *buf);

/**
 * @brief Set VBE video mode.
 * @param mode The video mode to set.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_set_vbe_video_mode(uint16_t mode);

/**
 * @brief Set VBE display start.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_set_vbe_display_start(uint16_t x, uint16_t y);

/**
 * @brief Set VBE display start at retrace.
 * @param x The x-coordinate.
 * @param y The y-coordinate.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_set_vbe_display_start_at_retrace(uint16_t x, uint16_t y);

/**
 * @brief Schedule VBE display start.
 * @param fboffset The framebuffer offset.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_schedule_vbe_display_start(uint32_t fboffset);

/**
 * @brief Schedule VBE display start at retrace.
 * @param fboffset The framebuffer offset.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_schedule_vbe_display_start_at_retrace(uint32_t fboffset);

/**
 * @brief Get VBE protected mode interface.
 * @param pmi_table A pointer to store the protected mode interface table.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_get_vbe_pm_interface(farptr_t *pmi_table);

/**
 * @brief Get VBE EDID information.
 * @param ctrlr_unit The controller unit.
 * @param edid_block The EDID block.
 * @param buf A pointer to the buffer to store the EDID information.
 * @return 0 on success, otherwise an error code.
 */
int _pc_bios_get_vbe_edid(uint16_t ctrlr_unit, uint16_t edid_block, struct edid *buf);

#endif // __I686_PC_BIOS_VIDEO_H__

