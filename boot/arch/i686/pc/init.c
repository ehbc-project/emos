/*
    From now on, What should we do is initialize the devices needed to load the
    kernel then load and jump to it. There's no need to detect and initialize
    all devices, since the kernel will do the rest.
*/

#include <stdint.h>
#include <stddef.h>

#include "asm/bootinfo.h"
#include "asm/bios/disk.h"
#include "asm/bios/video.h"
#include "asm/bios/keyboard.h"
#include "asm/bios/mem.h"

#include "bus/pci/scan.h"

#include "acpi/rsdp.h"
#include "acpi/rsdt.h"
#include "acpi/fadt.h"

#include "term/ansi.h"

extern void main(void);

extern int font_is_glyph_full_width(uint32_t codepoint);
extern int font_get_glyph(uint32_t codepoint, uint8_t *buf, long size);

struct pci_device pci_devices[32];

static void convert_color(struct vbe_video_mode_info *vbe_mode_info, uint32_t color, uint32_t *converted)
{
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    r >>= 8 - vbe_mode_info->red_mask;
    g >>= 8 - vbe_mode_info->green_mask;
    b >>= 8 - vbe_mode_info->blue_mask;

    *converted = (r << vbe_mode_info->red_position) |
                 (g << vbe_mode_info->green_position) |
                 (b << vbe_mode_info->blue_position);
}

static void print_glyph(struct ansi_state *state, uint32_t codepoint, uint32_t foreground, uint32_t background, uint16_t x_offset, uint16_t y_offset)
{
    struct vbe_video_mode_info *vbe_mode_info = state->data;

    int is_full_width = font_is_glyph_full_width(codepoint);
    uint8_t glyph[32];

    if (font_get_glyph(codepoint, glyph, 32)) return;

    uint8_t *framebuffer = (uint8_t*)(vbe_mode_info->framebuffer);

    uint32_t foreground_converted, background_converted;
    convert_color(vbe_mode_info, foreground, &foreground_converted);
    convert_color(vbe_mode_info, background, &background_converted);

    for (int y = y_offset; y < y_offset + 16; y++) {
        for (int x = x_offset; x < x_offset + (is_full_width ? 16 : 8); x++) {
            uint8_t current_byte = glyph[(y - y_offset) * (is_full_width ? 2 : 1) + (x - x_offset) / 8];
            int fg = current_byte & (0x80 >> ((x - x_offset) % 8));
            uint32_t* pixel = (uint32_t*)(framebuffer + y * vbe_mode_info->pitch + x * vbe_mode_info->bpp / 8);
            *pixel &= ~((1 << vbe_mode_info->bpp) - 1);

            if (state->text_reversed) {
                *pixel |= fg ? background_converted : foreground_converted;
            } else {
                *pixel |= fg ? foreground_converted : background_converted;
            }

            /*
            if (on) {
                *pixel |= ((1 << vbe_mode_info->blue_mask) - 1) << vbe_mode_info->blue_position;
                *pixel |= ((1 << vbe_mode_info->green_mask) - 1) << vbe_mode_info->green_position;
                *pixel |= ((1 << vbe_mode_info->red_mask) - 1) << vbe_mode_info->red_position;
            } else {
                *pixel &= ~(((1 << vbe_mode_info->blue_mask) - 1) << vbe_mode_info->blue_position);
                *pixel &= ~(((1 << vbe_mode_info->green_mask) - 1) << vbe_mode_info->green_position);
                *pixel &= ~(((1 << vbe_mode_info->red_mask) - 1) << vbe_mode_info->red_position);
            }
            */
        }

        if ((state->text_overlined && y == y_offset + 1) ||
            (state->text_strike && y == y_offset + 8) ||
            (state->text_underline && y == y_offset + 15)) {
            for (int x = x_offset; x < x_offset + (is_full_width ? 16 : 8); x++) {
                uint32_t *pixel = (uint32_t*)(framebuffer + y * vbe_mode_info->pitch + x * vbe_mode_info->bpp / 8);
                *pixel &= ~((1 << vbe_mode_info->bpp) - 1);
                *pixel |= state->text_reversed ? background : foreground;
            }
        }
    }
}

static void print_char(struct ansi_state *state, char ch)
{
    ansi_putchar(state, ch);
}

static void print_hex_dword(struct ansi_state *state, uint32_t value)
{
    static const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 0; i < 8; i++) {
        print_char(state, hex_chars[(value >> (28 - i * 4)) & 0xF]);
    }
}

static void print_dec(struct ansi_state *state, int value)
{
    if (value < 0) {
        ansi_putchar(state, '-');
        value = -value;
    } else if (value == 0) {
        ansi_putchar(state, '0');
        return;
    }

    int digits[10];
    int i = 0;
    while (value > 0) {
        digits[i++] = value % 10;
        value /= 10;
    }

    for (int j = i - 1; j >= 0; j--) {
        ansi_putchar(state, digits[j] + '0');
    }
}

static void print_str(struct ansi_state *state, const char *str)
{
    for (int i = 0; str[i]; i++) {
        ansi_putchar(state, str[i]);
    }
}

void term_draw_cursor(struct ansi_state *state)
{
    struct vbe_video_mode_info *vbe_mode_info = state->data;

    for (int y = state->cursor_y * 16 + 12; y < (state->cursor_y + 1) * 16; y++) {
        for (int x = state->cursor_x * 8; x < (state->cursor_x + 1) * 8; x++) {
            uint32_t *pixel = (uint32_t*)(vbe_mode_info->framebuffer + y * vbe_mode_info->pitch + x * vbe_mode_info->bpp / 8);
            *pixel |= 0xFFFFFF;
        }
    }
}

void term_erase(struct ansi_state *state, int x0, int y0, int x1, int y1)
{
    struct vbe_video_mode_info *vbe_mode_info = state->data;

    for (int y = y0 * 16; y < (y1 + 1) * 16; y++) {
        for (int x = x0 * 8; x < (x1 + 1) * 8; x++) {
            uint32_t *pixel = (uint32_t*)(vbe_mode_info->framebuffer + y * vbe_mode_info->pitch + x * vbe_mode_info->bpp / 8);
            *pixel &= ~0xFFFFFF;
            *pixel |= state->bg_color;
        }
    }
}

void term_putchar(struct ansi_state *state, char ch)
{
    print_glyph(state, ch, state->fg_color, state->bg_color, state->cursor_x * 8, state->cursor_y * 16);
}

void term_scroll(struct ansi_state *state, int n, int x0, int y0, int x1, int y1)
{
    struct vbe_video_mode_info *vbe_mode_info = state->data;

    if (y1 - y0 < n) {  // clear
        term_erase(state, x0, y0, x1, y1);
        return;
    }

    int start, end, direction;
    if (n < 0) {
        start = (y1 + 1) * 16;
        end = (y0 + n) * 16;
        direction = -1;
    } else {
        start = y0 * 16;
        end = (y1 + 1 - n) * 16;
        direction = 1;
    }

    for (int y = start; y != end; y += direction) {
        for (int x = x0 * 8; x <= (x1 + 1) * 8; x++) {
            uint32_t *from = (uint32_t*)(vbe_mode_info->framebuffer + (y + n * 16) * vbe_mode_info->pitch + x * vbe_mode_info->bpp / 8);
            uint32_t *to = (uint32_t*)(vbe_mode_info->framebuffer + y * vbe_mode_info->pitch + x * vbe_mode_info->bpp / 8);
            *to &= ~0xFFFFFF;
            *to |= *from & 0xFFFFFF;
        }
    }

    if (n > 0) {
        term_erase(state, x0, y1 - n + 1, x1, y1);
    } else {
        term_erase(state, x0, y0 + 1, x1, y0 - n);
    }
}

static void echo_char(struct ansi_state *state)
{
    char ch;
    for (;;) {
        _pc_bios_read_keyboard(NULL, &ch);
        if (ch == '\r') {
            print_str(state, "\r\n> ");
        } else if (ch == '\b') {
            ansi_putchar(state, 0x7F);
        } else {
            ansi_putchar(state, ch);
        }
    }
}

static int strncmp(const char *str1, const char *str2, int len)
{
    for (int i = 0; i < len; i++) {
        if (str1[i] != str2[i]) {
            return str1[i] - str2[i];
        }
    }
    return 0;
}

static struct acpi_fadt *find_fadt(struct acpi_rsdt *rsdt)
{
    for (int i = 0; i < (rsdt->header.length - sizeof(rsdt->header)) / sizeof(uint32_t); i++) {
        struct acpi_sdt_header *header = (struct acpi_sdt_header *)(rsdt->table_pointers[i]);
        if (strncmp(header->signature, ACPI_FADT_SIGNATURE, 4) == 0) {
            return (struct acpi_fadt *)header;
        }
    }
    return NULL;
}

__attribute__((noreturn))
void _pc_init(void)
{
    const char str[] = "Hello, World!\r\n";
    for (int i = 0; str[i]; i++) {
        _pc_bios_tty_output(str[i]);
    }

    pci_host_scan(pci_devices, 32);

    struct vbe_controller_info vbe_info;
    _pc_bios_get_vbe_controller_info(&vbe_info);

    uint16_t *mode_list = (uint16_t*)((vbe_info.video_modes.segment << 4) + vbe_info.video_modes.offset);
    struct vbe_video_mode_info vbe_mode_info;
    uint16_t mode = 0xFFFF;
    for (int i = 0; mode_list[i] != 0xFFFF; i++) {
        _pc_bios_get_vbe_video_mode_info(mode_list[i], &vbe_mode_info);

        if (vbe_mode_info.width == 1280 && vbe_mode_info.height == 1024 && vbe_mode_info.bpp == 24) {
            mode = mode_list[i];
            break;
        }
    }

    if (mode == 0xFFFF) {
        for (;;) {}
    }

    _pc_bios_set_vbe_video_mode(mode);

    struct ansi_state state;
    ansi_init_state(&vbe_mode_info, vbe_mode_info.width / 8, vbe_mode_info.height / 16, &state);

    uint32_t cursor = 0;
    struct smap_entry entry;

    do {
        _pc_bios_query_address_map(&cursor, &entry, sizeof(entry));
        print_hex_dword(&state, entry.base_addr_low);
        print_char(&state, ' ');
        print_hex_dword(&state, entry.length_low);
        print_char(&state, ' ');
        print_hex_dword(&state, entry.type);
        print_str(&state, "\r\n");
    } while (cursor);

    struct acpi_rsdp *rsdp = acpi_rsdp_find();
    if (rsdp) {
        print_str(&state, "RSDP found at ");
        print_hex_dword(&state, (uint32_t)rsdp);
        print_str(&state, "\r\n");
        print_str(&state, "\tVersion: ");
        print_dec(&state, rsdp->revision);
        switch (rsdp->revision) {
            case 0:
                print_str(&state, " ACPI 1.0\r\n");
                break;
            case 2:
                print_str(&state, " ACPI 2.0 or upper\r\n");
                break;
            default:
                print_str(&state, " Unknown\r\n");
                break;
        }
        print_str(&state, "\tOEM ID: ");
        print_str(&state, rsdp->oem_id);
        print_str(&state, "\r\n");

        struct acpi_rsdt *rsdt = (struct acpi_rsdt *)rsdp->rsdt_address;
        print_str(&state, "\tRSDT: ");
        print_hex_dword(&state, (uint32_t)rsdt);
        print_str(&state, "\r\n");
        print_str(&state, "\t\tCreator ID: ");
        print_str(&state, (const char *)&rsdt->header.creator_id);
        print_str(&state, "\r\n");
        print_str(&state, "\t\tCreator Revision: ");
        print_hex_dword(&state, rsdt->header.creator_revision);
        print_str(&state, "\r\n");
        print_str(&state, "\t\tLength: ");
        print_hex_dword(&state, rsdt->header.length);
        print_str(&state, "\r\n");

        print_str(&state, "\tXSDT: ");
        print_hex_dword(&state, rsdp->xsdt_address);
        print_str(&state, "\r\n");

        print_str(&state, "\tTables: \r\n");
        for (int i = 0; i < (rsdt->header.length - sizeof(rsdt->header)) / sizeof(uint32_t); i++) {
            struct acpi_sdt_header *header = (struct acpi_sdt_header *)(rsdt->table_pointers[i]);
            print_str(&state, "\t- ");
            print_str(&state, header->signature);
            print_str(&state, "\r\n");
        }

        struct acpi_fadt *fadt = find_fadt(rsdt);
        if (fadt) {
            print_str(&state, "\tFADT: ");
            print_hex_dword(&state, (uint32_t)fadt);
            print_str(&state, "\r\n");

            print_str(&state, "\t\t\033[4mACPI Enable: ");
            print_dec(&state, fadt->acpi_enable);
            print_str(&state, "\r\n");
            print_str(&state, "\t\t\033[9mACPI Disable: ");
            print_dec(&state, fadt->acpi_disable);
            print_str(&state, "\r\n");

            print_str(&state, "\t\t\033[24mPM1a Event Block: ");
            print_hex_dword(&state, fadt->pm1a_event_block);
            print_str(&state, "\r\n");
            print_str(&state, "\t\t\033[29m\033[53mPM1b Event Block: ");
            print_hex_dword(&state, fadt->pm1b_event_block);
            print_str(&state, "\r\n");

            print_str(&state, "\t\t\033[9mPM1a Control Block: ");
            print_hex_dword(&state, fadt->pm1a_control_block);
            print_str(&state, "\r\n");
            print_str(&state, "\t\t\033[4m\033[29m\033[55mPM1b Control Block: ");
            print_hex_dword(&state, fadt->pm1b_control_block);
            print_str(&state, "\r\n");

            print_str(&state, "\t\t\033[24m\033[7mPM2 Control Block: ");
            print_hex_dword(&state, fadt->pm2_control_block);
            print_str(&state, "\r\n");

            print_str(&state, "\t\t\033[27mPM Timer Block: ");
            print_hex_dword(&state, fadt->pm_timer_block);
            print_str(&state, "\r\n");
        }
    }

    for (int i = 0; i < 16; i++) {
        for (int j = 0; j < 16; j++) {
            print_str(&state, "\033[48;5;");
            print_dec(&state, i * 16 + j);
            print_str(&state, "m color ");
        }
        print_str(&state, "\r\n");
    }

    print_str(&state, "\033[0m");
    echo_char(&state);

    main();

    for (;;) {}
}
