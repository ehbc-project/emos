#ifndef __DEVICE_VIDEO_VBE_H__
#define __DEVICE_VIDEO_VBE_H__

#include <stdint.h>

#include <asm/farptr.h>
#include <device/device.h>
#include <interface/framebuffer.h>

#define VBE_CTRL_SIGNATURE_VBE1 "VESA"
#define VBE_CTRL_SIGNATURE_VBE2 "VBE2"

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


struct vbe_device {
    struct device dev;

    const struct framebuffer_interface *fbif;

    uint32_t *frame_buffer;
    uint8_t *diff_buffer;
    int current_page;

    struct vbe_video_mode_info mode_info;
};



#endif // __DEVICE_VIDEO_VBE_H__
