#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <wchar.h>

#include <eboot/macros.h>
#include <eboot/shell.h>
#include <eboot/device.h>
#include <eboot/hid.h>
#include <eboot/font.h>
#include <eboot/interface/video.h>
#include <eboot/interface/framebuffer.h>
#include <eboot/interface/hid.h>

static struct device *fbdev;
static const struct video_interface *vidif;
static const struct framebuffer_interface *fbif;
static struct device *kbdev;
static const struct hid_interface *kbhidif;
static struct device *msdev;
static const struct hid_interface *mshidif;
static int current_vmode;
static struct video_mode_info vmode_info;
static uint32_t *framebuffer;

static const uint32_t color_palette[16] = {
    0x000000, 0x000080, 0x008000, 0x008080, 0x800000, 0x800080, 0x808000, 0xC0C0C0,
    0x808080, 0x0000FF, 0x00FF00, 0x00FFFF, 0xFF0000, 0xFF00FF, 0xFFFF00, 0xFFFFFF,
};

static void draw_line(int x0, int y0, int x1, int y1, uint32_t color)
{
    if (x0 == x1) {
        for (int y = y0; y <= y1; y++) {
            framebuffer[y * vmode_info.width + x0] = color;
        }
    } else if (y0 == y1) {
        for (int x = x0; x <= x1; x++) {
            framebuffer[y0 * vmode_info.width + x] = color;
        }
    } else {
        int dx =  abs (x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -abs (y1 - y0), sy = y0 < y1 ? 1 : -1; 
        int err = dx + dy, e2;
       
        for (;;) {
            framebuffer[y0 * vmode_info.width + x0] = color;
            if (x0 == x1 && y0 == y1) break;
            e2 = 2 * err;
            if (e2 >= dy) {
                err += dy;
                x0 += sx;
            }
            if (e2 <= dx) {
                err += dx;
                y0 += sy;
            }
        }
    }
}

static inline uint32_t blend_color(uint32_t upper, uint32_t lower)
{
    uint32_t new_color;

    int alpha = (upper >> 24) & 0xFF;

    if (!alpha) {
        new_color = lower;
    } else if (alpha == 0xFF) {
        new_color = upper & 0xFFFFFF;
    } else {
        int upper_r = (upper >> 16) & 0xFF;
        int upper_g = (upper >> 8) & 0xFF;
        int upper_b = upper & 0xFF;
        int lower_r = (lower >> 16) & 0xFF;
        int lower_g = (lower >> 8) & 0xFF;
        int lower_b = lower & 0xFF;

        new_color =
            ((lower_r * (255 - alpha) / 255 + upper_r * alpha / 255) << 16) |
            ((lower_g * (255 - alpha) / 255 + upper_g * alpha / 255) << 8) |
            (lower_b * (255 - alpha) / 255 + upper_b * alpha / 255);
    }

    return new_color;
}

static void draw_rect(int xpos, int ypos, int width, int height, uint32_t color, int fill)
{
    for (int x = xpos; x < xpos + width; x++) {
        framebuffer[ypos * vmode_info.width + x] = color;
    }

    for (int y = ypos + 1; y < ypos + height - 1; y++) {
        if (fill) {
            for (int x = xpos; x < xpos + width; x++) {
                framebuffer[y * vmode_info.width + x] = color;
            }
        } else {
            framebuffer[y * vmode_info.width + xpos] = color;
            framebuffer[y * vmode_info.width + xpos + width - 1] = color;
        }
    }

    for (int x = xpos; x < xpos + width; x++) {
        framebuffer[(ypos + height - 1) * vmode_info.width + x] = color;
    }
}

static void draw_button_frame(int xpos, int ypos, int width, int height)
{
    draw_line(xpos, ypos, xpos + width - 2, ypos, color_palette[15]);
    draw_line(xpos, ypos, xpos, ypos + height - 2, color_palette[15]);
    draw_line(xpos + width - 2, ypos + 1, xpos + width - 2, ypos + height - 2, color_palette[8]);
    draw_line(xpos + 1, ypos + height - 2, xpos + width - 3, ypos + height - 2, color_palette[8]);
    draw_line(xpos + width - 1, ypos, xpos + width - 1, ypos + height - 1, color_palette[0]);
    draw_line(xpos, ypos + height - 1, xpos + width - 1, ypos + height - 1, color_palette[0]);
    draw_rect(xpos + 1, ypos + 1, width - 3, height - 3, color_palette[7], 1);
}

static const uint32_t x_icon[] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFF000000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000,
};

static void draw_image(int xpos, int ypos, int width, int height, const uint32_t *img)
{
    for (int y = ypos; y < ypos + height; y++) {
        for (int x = xpos; x < xpos + width; x++) {
            framebuffer[y * vmode_info.width + x] = blend_color(img[(y - ypos) * width + x - xpos], framebuffer[y * vmode_info.width + x]);
        }
    }
}

inline static int get_glyph_bit(const uint8_t *glyph_data, int cwidth, int x, int y)
{
    return glyph_data[(y * cwidth + x) / 8] & (0x80 >> ((y * cwidth + x) % 8));
}

static void draw_char(int xpos, int ypos, uint32_t color, wchar_t ch)
{
    status_t status;
    int use_fallback;
    int cwidth, cheight;
    const uint8_t *glyph;
    uint8_t font_glyph[32];
    static const uint8_t fallback_glyph[16] = {
        0x00, 0x00, 0x00, 0x7E, 0x66, 0x5A, 0x5A, 0x7A,
        0x76, 0x76, 0x7E, 0x76, 0x76, 0x7E, 0x00, 0x00,
    };

    use_fallback = 0;

    status = font_get_glyph_dimension(ch, &cwidth, &cheight);
    if (!CHECK_SUCCESS(status) || (cwidth != 8 && cwidth != 16) || cheight != 16) {
        use_fallback = 1;
    }

    if (!use_fallback) {
        status = font_get_glyph_data(ch, font_glyph, sizeof(font_glyph));
        if (!CHECK_SUCCESS(status)) {
            use_fallback = 1;
        }
    }

    if (use_fallback) {
        cwidth = 8;
        cheight = 16;
    }

    glyph = use_fallback ? fallback_glyph : font_glyph;

    for (int y = 0; y < cheight; y++) {
        for (int x = 0; x < cwidth; x++) {
            int draw = get_glyph_bit(glyph, cwidth, x, y);

            if (x > 0) {
                draw = draw || get_glyph_bit(glyph, cwidth, x - 1, y);
            }

            if (draw) {
                framebuffer[(ypos + y) * vmode_info.width + xpos + x] = color;
            }
        }
    }
}

static void draw_text(int xpos, int ypos, uint32_t color, const wchar_t *str)
{
    while (*str) {
        int cwidth = wcwidth(*str) * 8;

        draw_char(xpos, ypos, color, *str);

        xpos += cwidth;
        str++;
    }
}

static void draw_window(int xpos, int ypos, int width, int height)
{
    draw_rect(xpos, ypos, width - 1, height - 1, color_palette[8], 0);
    draw_line(xpos + 1, ypos + 1, xpos + width - 2, ypos + 1, color_palette[15]);
    draw_line(xpos + 1, ypos + 2, xpos + 1, ypos + height - 3, color_palette[15]);
    draw_line(xpos, ypos + height - 1, xpos + width - 1, ypos + height - 1, color_palette[0]);
    draw_line(xpos + width - 1, ypos, xpos + width - 1, ypos + height - 2, color_palette[0]);
    draw_rect(xpos + 2, ypos + 2, width - 4, height - 4, color_palette[7], 1);

    draw_rect(xpos + 4, ypos + 4, width - 8, 20, color_palette[1], 1);
    draw_text(xpos + 8, ypos + 6, color_palette[15], L"Hello, World!");
    draw_button_frame(xpos + width - 22, ypos + 6, 16, 16);
    draw_image(xpos + width - 20, ypos + 8, 12, 10, x_icon);
    draw_button_frame(xpos + width - 40, ypos + 6, 16, 16);
    draw_button_frame(xpos + width - 56, ypos + 6, 16, 16);
}

static void draw_frame(int xpos, int ypos, int width, int height)
{
    draw_line(xpos, ypos, xpos + width - 2, ypos, color_palette[8]);
    draw_line(xpos, ypos, xpos, ypos + height - 2, color_palette[8]);
    draw_line(xpos + width - 1, ypos, xpos + width - 1, ypos + height - 1, color_palette[15]);
    draw_line(xpos, ypos + height - 1, xpos + width - 1, ypos + height - 1, color_palette[15]);
    draw_rect(xpos + 1, ypos + 1, width - 2, height - 2, color_palette[7], 1);
}

static status_t mouse_move_to(int xpos, int ypos)
{
    status_t status;

    static const uint32_t cursor_data[19][12] = {
        { 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, },
        { 0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, },
        { 0xFF000000, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, },
        { 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, },
        { 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, },
        { 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, },
        { 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, },
        { 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, },
        { 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, },
        { 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, },
        { 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, },
        { 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, },
        { 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, 0xFF000000, },
        { 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, },
        { 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, },
        { 0xFF000000, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, },
        { 0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, },
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFFFFFFFF, 0xFFFFFFFF, 0xFF000000, 0x00000000, 0x00000000, },
        { 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0x00000000, 0xFF000000, 0xFF000000, 0x00000000, 0x00000000, 0x00000000, },
    };
    static int has_drawn_before = 0;
    static int prev_xpos, prev_ypos;
    static uint32_t prev_cursor_area_data[19][12];

    if (has_drawn_before) {
        for (int y = prev_ypos; y < MIN(prev_ypos + 19, vmode_info.height); y++) {
            for (int x = prev_xpos; x < MIN(prev_xpos + 12, vmode_info.width); x++) {
                framebuffer[y * vmode_info.width + x] = prev_cursor_area_data[y - prev_ypos][x - prev_xpos];
            }
        }

        status = fbif->invalidate(fbdev, prev_xpos, prev_ypos, prev_xpos + 12, prev_ypos + 19);
        if (!CHECK_SUCCESS(status)) return status;
    }

    prev_xpos = xpos;
    prev_ypos = ypos;
    for (int y = ypos; y < MIN(ypos + 19, vmode_info.height); y++) {
        for (int x = xpos; x < MIN(xpos + 12, vmode_info.width); x++) {
            uint32_t cursor_pixel = cursor_data[y - ypos][x - xpos];
            uint32_t fb_pixel = framebuffer[y * vmode_info.width + x];

            prev_cursor_area_data[y - ypos][x - xpos] = fb_pixel;

            framebuffer[y * vmode_info.width + x] = blend_color(cursor_pixel, fb_pixel);
        }
    }

    has_drawn_before = 1;

    status = fbif->invalidate(fbdev, xpos, ypos, xpos + 12, ypos + 19);
    if (!CHECK_SUCCESS(status)) return status;

    status = fbif->flush(fbdev);
    if (!CHECK_SUCCESS(status)) return status;

    return STATUS_SUCCESS;
}

static int guishell_handler(struct shell_instance *inst, int argc, char **argv)
{
    status_t status;
    uint16_t key, flags;
    int mouse_xpos, mouse_ypos, should_exit;

    status = device_find("video0", &fbdev);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "%s: cannot find device\n", argv[0]);
        return 1;
    }

    status = fbdev->driver->get_interface(fbdev, "video", (const void **)&vidif);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "%s: cannot get interface from device\n", argv[0]);
        return 1;
    }

    status = fbdev->driver->get_interface(fbdev, "framebuffer", (const void **)&fbif);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "%s: cannot get interface from device\n", argv[0]);
        return 1;
    }

    status = device_find("kbd0", &kbdev);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "%s: cannot find device\n", argv[0]);
        return 1;
    }
    
    status = kbdev->driver->get_interface(kbdev, "hid", (const void **)&kbhidif);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "%s: cannot get interface from device\n", argv[0]);
        return 1;
    }

    status = device_find("mouse0", &msdev);
    if (!CHECK_SUCCESS(status)) {
        msdev = NULL;
    }
    
    if (msdev) {
        status = msdev->driver->get_interface(msdev, "hid", (const void **)&mshidif);
        if (!CHECK_SUCCESS(status)) {
            fprintf(stderr, "%s: cannot get interface from device\n", argv[0]);
            return 1;
        }
    }

    status = fbif->get_framebuffer(fbdev, (void **)&framebuffer);
    if (!CHECK_SUCCESS(status)) return 1;

    status = vidif->get_mode(fbdev, &current_vmode);
    if (!CHECK_SUCCESS(status)) return 1;

    status = vidif->get_mode_info(fbdev, current_vmode, &vmode_info);
    if (!CHECK_SUCCESS(status)) return 1;

    draw_rect(0, 0, vmode_info.width, vmode_info.height, color_palette[3], 1);

    draw_window(50, 50, vmode_info.width - 100, vmode_info.height - 100);

    status = fbif->invalidate(fbdev, 0, 0, vmode_info.width - 1, vmode_info.height - 1);
    if (!CHECK_SUCCESS(status)) return 1;

    status = fbif->flush(fbdev);
    if (!CHECK_SUCCESS(status)) return 1;

    if (msdev) {
        mouse_xpos = vmode_info.width / 2;
        mouse_ypos = vmode_info.height / 2;
        mouse_move_to(mouse_xpos, mouse_ypos);
    }

    should_exit = 0;
    while (!should_exit) {
        status = kbhidif->poll_event(kbdev, &key, &flags);
        if (CHECK_SUCCESS(status) && status != STATUS_NO_EVENT) {
            if (flags & KEY_FLAG_BREAK) continue;

            switch (key) {
                case KEY_ESC:
                    should_exit = 1;
                    break;
                default:
                    break;
            }
        }

        if (!msdev) continue;

        status = mshidif->poll_event(msdev, &key, &flags);
        if (CHECK_SUCCESS(status) && status != STATUS_NO_EVENT) {
            switch (flags & KEY_FLAG_TYPEMASK) {
                case 0:
                    if (flags & KEY_FLAG_BREAK) break;
    
                    if (key == KEY_MOUSEBTNL) {
                        if (vmode_info.width - 72 <= mouse_xpos && mouse_xpos < vmode_info.width - 56 && 56 <= mouse_ypos && mouse_ypos < 80) {
                            should_exit = 1;
                        }
                    }
                    break;
                case KEY_FLAG_XMOVE:
                    if ((flags & KEY_FLAG_NEGATIVE)) {
                        if (mouse_xpos < key) {
                            mouse_xpos = 0;
                        } else {
                            mouse_xpos -= key;
                        }
                    } else if (!(flags & KEY_FLAG_NEGATIVE)) {
                        if (vmode_info.width <= mouse_xpos + key) {
                            mouse_xpos = vmode_info.width - 1;
                        } else {
                            mouse_xpos += key;
                        }
                    }
                    break;
                case KEY_FLAG_YMOVE:
                    if ((flags & KEY_FLAG_NEGATIVE)) {
                        if (vmode_info.height <= mouse_ypos + key) {
                            mouse_ypos = vmode_info.height - 1;
                        } else {
                            mouse_ypos += key;
                        }
                    } else if (!(flags & KEY_FLAG_NEGATIVE)) {
                        if (mouse_ypos < key) {
                            mouse_ypos = 0;
                        } else {
                            mouse_ypos -= key;
                        }
                    }

                    mouse_move_to(mouse_xpos, mouse_ypos);
                    break;
                default:
                    break;
            }
        }
    }

    return 0;
}

static struct command guishell_command = {
    .name = "guishell",
    .handler = guishell_handler,
    .help_message = "Run GUI shell",
};

__constructor
static void init()
{
    shell_command_register(&guishell_command);
}

status_t _start(int argc, char **argv)
{
    return STATUS_SUCCESS;
}

__destructor
static void deinit(void)
{
    shell_command_unregister(&guishell_command);
}
