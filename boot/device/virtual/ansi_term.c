#include <device/virtual/ansi_term.h>

#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <string.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <device/video/vconsole.h>
#include <interface/console.h>
#include <interface/char.h>

enum ansi_escape_state {
    STATE_DEFAULT = 0,

    STATE_ESC, /* -> SS2, SS3, CSI, ST, OSC, SOS, PM, APC */

    STATE_SS2, 
    STATE_SS3,
    STATE_DCS,
    STATE_CSI,
    STATE_ST,
    STATE_OSC,
    STATE_SOS,
    STATE_PM,
    STATE_APC,
};

static const uint32_t palette[256] = {
    /* RGBI colors */
    0x000000, 0xAA0000, 0x00AA00, 0xAA5500,
    0x0000AA, 0xAA00AA, 0x00AAAA, 0xAAAAAA,
    0x555555, 0xFF5555, 0x55FF55, 0xFFFF55,
    0x5555FF, 0xFF55FF, 0x55FFFF, 0xFFFFFF,

    /* 216 colors */
    0x000000, 0x00005F, 0x000087, 0x0000AF, 
    0x0000D7, 0x0000FF, 0x005F00, 0x005F5F, 
    0x005F87, 0x005FAF, 0x005FD7, 0x005FFF, 
    0x008700, 0x00875F, 0x008787, 0x0087AF, 
    0x0087D7, 0x0087FF, 0x00AF00, 0x00AF5F, 
    0x00AF87, 0x00AFAF, 0x00AFD7, 0x00AFFF, 
    0x00D700, 0x00D75F, 0x00D787, 0x00D7AF, 
    0x00D7D7, 0x00D7FF, 0x00FF00, 0x00FF5F, 
    0x00FF87, 0x00FFAF, 0x00FFD7, 0x00FFFF, 
    0x5F0000, 0x5F005F, 0x5F0087, 0x5F00AF, 
    0x5F00D7, 0x5F00FF, 0x5F5F00, 0x5F5F5F, 
    0x5F5F87, 0x5F5FAF, 0x5F5FD7, 0x5F5FFF, 
    0x5F8700, 0x5F875F, 0x5F8787, 0x5F87AF, 
    0x5F87D7, 0x5F87FF, 0x5FAF00, 0x5FAF5F, 
    0x5FAF87, 0x5FAFAF, 0x5FAFD7, 0x5FAFFF, 
    0x5FD700, 0x5FD75F, 0x5FD787, 0x5FD7AF, 
    0x5FD7D7, 0x5FD7FF, 0x5FFF00, 0x5FFF5F, 
    0x5FFF87, 0x5FFFAF, 0x5FFFD7, 0x5FFFFF, 
    0x870000, 0x87005F, 0x870087, 0x8700AF, 
    0x8700D7, 0x8700FF, 0x875F00, 0x875F5F, 
    0x875F87, 0x875FAF, 0x875FD7, 0x875FFF, 
    0x878700, 0x87875F, 0x878787, 0x8787AF, 
    0x8787D7, 0x8787FF, 0x87AF00, 0x87AF5F, 
    0x87AF87, 0x87AFAF, 0x87AFD7, 0x87AFFF, 
    0x87D700, 0x87D75F, 0x87D787, 0x87D7AF, 
    0x87D7D7, 0x87D7FF, 0x87FF00, 0x87FF5F, 
    0x87FF87, 0x87FFAF, 0x87FFD7, 0x87FFFF, 
    0xAF0000, 0xAF005F, 0xAF0087, 0xAF00AF, 
    0xAF00D7, 0xAF00FF, 0xAF5F00, 0xAF5F5F, 
    0xAF5F87, 0xAF5FAF, 0xAF5FD7, 0xAF5FFF, 
    0xAF8700, 0xAF875F, 0xAF8787, 0xAF87AF, 
    0xAF87D7, 0xAF87FF, 0xAFAF00, 0xAFAF5F, 
    0xAFAF87, 0xAFAFAF, 0xAFAFD7, 0xAFAFFF, 
    0xAFD700, 0xAFD75F, 0xAFD787, 0xAFD7AF, 
    0xAFD7D7, 0xAFD7FF, 0xAFFF00, 0xAFFF5F, 
    0xAFFF87, 0xAFFFAF, 0xAFFFD7, 0xAFFFFF, 
    0xD70000, 0xD7005F, 0xD70087, 0xD700AF, 
    0xD700D7, 0xD700FF, 0xD75F00, 0xD75F5F, 
    0xD75F87, 0xD75FAF, 0xD75FD7, 0xD75FFF, 
    0xD78700, 0xD7875F, 0xD78787, 0xD787AF, 
    0xD787D7, 0xD787FF, 0xD7AF00, 0xD7AF5F, 
    0xD7AF87, 0xD7AFAF, 0xD7AFD7, 0xD7AFFF, 
    0xD7D700, 0xD7D75F, 0xD7D787, 0xD7D7AF, 
    0xD7D7D7, 0xD7D7FF, 0xD7FF00, 0xD7FF5F, 
    0xD7FF87, 0xD7FFAF, 0xD7FFD7, 0xD7FFFF, 
    0xFF0000, 0xFF005F, 0xFF0087, 0xFF00AF, 
    0xFF00D7, 0xFF00FF, 0xFF5F00, 0xFF5F5F, 
    0xFF5F87, 0xFF5FAF, 0xFF5FD7, 0xFF5FFF, 
    0xFF8700, 0xFF875F, 0xFF8787, 0xFF87AF, 
    0xFF87D7, 0xFF87FF, 0xFFAF00, 0xFFAF5F, 
    0xFFAF87, 0xFFAFAF, 0xFFAFD7, 0xFFAFFF, 
    0xFFD700, 0xFFD75F, 0xFFD787, 0xFFD7AF, 
    0xFFD7D7, 0xFFD7FF, 0xFFFF00, 0xFFFF5F, 
    0xFFFF87, 0xFFFFAF, 0xFFFFD7, 0xFFFFFF,

    /* grayscale colors */
    0x080808, 0x121212, 0x1C1C1C, 0x262626, 
    0x303030, 0x3A3A3A, 0x444444, 0x4E4E4E, 
    0x585858, 0x626262, 0x6C6C6C, 0x767676, 
    0x808080, 0x8A8A8A, 0x949494, 0x9E9E9E, 
    0xA8A8A8, 0xB2B2B2, 0xBCBCBC, 0xC6C6C6, 
    0xD0D0D0, 0xDADADA, 0xE4E4E4, 0xEEEEEE,
};

static const struct console_char_attributes default_attr = {
    .fg_color = 0xffffff,
    .bg_color = 0x000000,
    .text_blink_level = 0,
    .text_bold = 0,
    .text_dim = 0,
    .text_italic = 0,
    .text_underline = 0,
    .text_strike = 0,
    .text_overlined = 0,
    .text_reversed = 0,
};

static void console_erase(struct ansi_term_device *dev, int x0, int y0, int x1, int y1)
{
    int width;
    dev->condev_conif->get_dimension(dev->condev, &width, NULL);
    struct console_char_cell *buffer = dev->condev_conif->get_buffer(dev->condev);

    for (int y = y0; y <= y1; y++) {
        for (int x = x0; x <= x1; x++) {
            buffer[y * width + x].attr = dev->attr_state;
            buffer[y * width + x].codepoint = 0;
        }
    }

    dev->condev_conif->invalidate(dev->condev, x0, y0, x1, y1);
}

static void console_scroll(struct ansi_term_device *dev, int amount, int x0, int y0, int x1, int y1)
{
    int width;
    dev->condev_conif->get_dimension(dev->condev, &width, NULL);
    struct console_char_cell *buffer = dev->condev_conif->get_buffer(dev->condev);

    if (amount == 0) return;

    if (amount > 0) {
        for (int y = y0; y <= y1 - amount; y++) {
            for (int x = x0; x <= x1; x++) {
                buffer[y * width + x] = buffer[(y + amount) * width + x];
            }
        }
        
        console_erase(dev, x0, y1 - amount + 1, x1, y1);
    } else { 
        amount = -amount;

        for (int y = y1 - amount; y >= y0; y--) {
            for (int x = x0; x <= x1; x++) {
                buffer[(y + amount) * width + x] = buffer[y * width + x];
            }
        }
        
        console_erase(dev, x0, y0, x1, y0 + amount);
    }

    dev->condev_conif->invalidate(dev->condev, x0, y0, x1, y1);
}

static void console_putchar(struct ansi_term_device *dev, char ch)
{
    int x_cursor, y_cursor, width;
    dev->condev_conif->get_cursor_pos(dev->condev, &x_cursor, &y_cursor);
    dev->condev_conif->get_dimension(dev->condev, &width, NULL);
    struct console_char_cell *buffer = dev->condev_conif->get_buffer(dev->condev);

    buffer[y_cursor * width + x_cursor].attr = dev->attr_state;
    buffer[y_cursor * width + x_cursor].codepoint = ch;
    dev->condev_conif->invalidate(dev->condev, x_cursor, y_cursor, x_cursor, y_cursor);
}

static void handle_esc(struct ansi_term_device *dev, char ch)
{
    for (int i = 0; i < 8; i++) {
        dev->escape_seq_args[i] = 0;
    }
    dev->escape_seq_arg_count = 0;
    dev->escape_seq_number_input = 0;
    switch (ch) {
        case 'N':
            dev->escape_seq_state = STATE_SS2;
            break;
        case 'O':
            dev->escape_seq_state = STATE_SS3;
            break;
        case 'P':
            dev->escape_seq_state = STATE_DCS;
            break;
        case '[':
            dev->escape_seq_state = STATE_CSI;
            break;
        case '\\':
            dev->escape_seq_state = STATE_ST;
            break;
        case ']':
            dev->escape_seq_state = STATE_OSC;
            break;
        case 'X':
            dev->escape_seq_state = STATE_SOS;
            break;
        case '^':
            dev->escape_seq_state = STATE_PM;
            break;
        case '_':
            dev->escape_seq_state = STATE_APC;
            break;
        default:
            dev->escape_seq_state = STATE_DEFAULT;
            break;
    }
}

static void handle_cuu(struct ansi_term_device *dev)
{
    int cursor_y;
    dev->condev_conif->get_cursor_pos(dev->condev, NULL, &cursor_y);
    
    int count = (dev->escape_seq_arg_count < 1) ? 1 : dev->escape_seq_args[0];
    count = (count < cursor_y) ? count : cursor_y;
    cursor_y -= count;
    dev->escape_seq_state = STATE_DEFAULT;

    dev->condev_conif->set_cursor_pos(dev->condev, -1, cursor_y);
}

static void handle_cud(struct ansi_term_device *dev)
{
    int cursor_y, height;
    dev->condev_conif->get_cursor_pos(dev->condev, NULL, &cursor_y);
    dev->condev_conif->get_dimension(dev->condev, NULL, &height);

    int count = (dev->escape_seq_arg_count < 1) ? 1 : dev->escape_seq_args[0];
    count = (count < height - cursor_y) ? count : height - cursor_y;
    cursor_y += count;
    dev->escape_seq_state = STATE_DEFAULT;

    dev->condev_conif->set_cursor_pos(dev->condev, -1, cursor_y);
}

static void handle_cuf(struct ansi_term_device *dev)
{
    int cursor_x, width;
    dev->condev_conif->get_cursor_pos(dev->condev, &cursor_x, NULL);
    dev->condev_conif->get_dimension(dev->condev, &width, NULL);
    
    int count = (dev->escape_seq_arg_count < 1) ? 1 : dev->escape_seq_args[0];
    count = (count < width - cursor_x) ? count : width - cursor_x;
    cursor_x += count;
    dev->escape_seq_state = STATE_DEFAULT;

    dev->condev_conif->set_cursor_pos(dev->condev, cursor_x, -1);
}

static void handle_cub(struct ansi_term_device *dev)
{
    int cursor_x;
    dev->condev_conif->get_cursor_pos(dev->condev, &cursor_x, NULL);

    int count = (dev->escape_seq_arg_count < 1) ? 1 : dev->escape_seq_args[0];
    count = (count < cursor_x) ? count : cursor_x;
    cursor_x -= count;
    dev->escape_seq_state = STATE_DEFAULT;
    
    dev->condev_conif->set_cursor_pos(dev->condev, cursor_x, -1);
}

static void handle_cnl(struct ansi_term_device *dev)
{
    int cursor_x, cursor_y, height;
    dev->condev_conif->get_cursor_pos(dev->condev, &cursor_x, &cursor_y);
    dev->condev_conif->get_dimension(dev->condev, NULL, &height);

    int count = (dev->escape_seq_arg_count < 1) ? 1 : dev->escape_seq_args[0];
    count = (count < height - cursor_y) ? count : height - cursor_y;
    cursor_y += count;
    cursor_x = 0;
    dev->escape_seq_state = STATE_DEFAULT;

    dev->condev_conif->set_cursor_pos(dev->condev, cursor_x, cursor_y);
}

static void handle_cpl(struct ansi_term_device *dev)
{
    int cursor_x, cursor_y;
    dev->condev_conif->get_cursor_pos(dev->condev, &cursor_x, &cursor_y);
    
    int count = (dev->escape_seq_arg_count < 1) ? 1 : dev->escape_seq_args[0];
    count = (count < cursor_y) ? count : cursor_y;
    cursor_y -= count;
    cursor_x = 0;
    dev->escape_seq_state = STATE_DEFAULT;

    dev->condev_conif->set_cursor_pos(dev->condev, cursor_x, cursor_y);
}

static void handle_cha(struct ansi_term_device *dev)
{
    int cursor_x;
    dev->condev_conif->get_cursor_pos(dev->condev, &cursor_x, NULL);

    int pos = (dev->escape_seq_arg_count < 1) ? 1 : dev->escape_seq_args[0];
    cursor_x = pos - 1;
    if (cursor_x < 0) {
        cursor_x = 0;
    }
    dev->escape_seq_state = STATE_DEFAULT;

    dev->condev_conif->set_cursor_pos(dev->condev, cursor_x, -1);
}

static void handle_cup_hvp(struct ansi_term_device *dev)
{
    int cursor_x, cursor_y;
    dev->condev_conif->get_cursor_pos(dev->condev, &cursor_x, &cursor_y);

    int ypos = (dev->escape_seq_arg_count < 1) ? 1 : dev->escape_seq_args[0];
    int xpos = (dev->escape_seq_arg_count < 2) ? 1 : dev->escape_seq_args[1];
    cursor_x = xpos - 1;
    if (cursor_x < 0) {
        cursor_x = 0;
    }
    cursor_y = ypos - 1;
    if (cursor_y < 0) {
        cursor_y = 0;
    }
    dev->escape_seq_state = STATE_DEFAULT;

    dev->condev_conif->set_cursor_pos(dev->condev, cursor_x, cursor_y);
}

static void handle_ed(struct ansi_term_device *dev)
{
    int cursor_x, cursor_y, width, height;
    dev->condev_conif->get_cursor_pos(dev->condev, &cursor_x, &cursor_y);
    dev->condev_conif->get_dimension(dev->condev, &width, &height);

    int option = (dev->escape_seq_arg_count < 1) ? 0 : dev->escape_seq_args[0];
    switch (option) {
        case 0:
            console_erase(dev, cursor_x, cursor_y, width - 1, cursor_y);
            if (cursor_y < height - 1) break;
            console_erase(dev, 0, cursor_y + 1, width - 1, height - 1);
            break;
        case 1:
            console_erase(dev, 0, cursor_y, cursor_x, cursor_y);
            if (cursor_x > 0) break;
            console_erase(dev, 0, 0, width - 1, cursor_y - 1);
            break;
        case 2:
        case 3:
            console_erase(dev, 0, 0, width - 1, height - 1);
            break;
    }
    dev->escape_seq_state = STATE_DEFAULT;
}

static void handle_el(struct ansi_term_device *dev)
{
    int cursor_x, cursor_y, width;
    dev->condev_conif->get_cursor_pos(dev->condev, &cursor_x, &cursor_y);
    dev->condev_conif->get_dimension(dev->condev, &width, NULL);

    int option = (dev->escape_seq_arg_count < 1) ? 0 : dev->escape_seq_args[0];
    switch (option) {
        case 0:
            console_erase(dev, cursor_x, cursor_y, width - 1, cursor_y);
            break;
        case 1:
            console_erase(dev, 0, cursor_y, cursor_x, cursor_y);
            break;
    }
    dev->escape_seq_state = STATE_DEFAULT;
}

static void handle_su(struct ansi_term_device *dev)
{
    int width, height;
    dev->condev_conif->get_dimension(dev->condev, &width, &height);

    int amount = (dev->escape_seq_arg_count < 1) ? 1 : dev->escape_seq_args[0];
    console_scroll(dev, amount, 0, 0, width - 1, height - 1);
    dev->escape_seq_state = STATE_DEFAULT;
}

static void handle_sd(struct ansi_term_device *dev)
{
    int width, height;
    dev->condev_conif->get_dimension(dev->condev, &width, &height);

    int amount = (dev->escape_seq_arg_count < 1) ? 1 : dev->escape_seq_args[0];
    console_scroll(dev, -amount, 0, 0, width - 1, height - 1);
    dev->escape_seq_state = STATE_DEFAULT;
}

static void handle_sgr(struct ansi_term_device *dev)
{
    int option = (dev->escape_seq_arg_count < 1) ? 0 : dev->escape_seq_args[0];
    switch (option) {
        case 0:
            dev->attr_state.bg_color = palette[0];
            dev->attr_state.fg_color = palette[15];
            dev->attr_state.text_blink_level = 0;
            dev->attr_state.text_bold = 0;
            dev->attr_state.text_dim = 0;
            dev->attr_state.text_italic = 0;
            dev->attr_state.text_underline = 0;
            dev->attr_state.text_strike = 0;
            dev->attr_state.text_overlined = 0;
            break;
        case 1:
            dev->attr_state.text_bold = 1;
            break;
        case 2:
            dev->attr_state.text_dim = 1;
            break;
        case 3:
            dev->attr_state.text_italic = 1;
            break;
        case 4:
            dev->attr_state.text_underline = 1;
            break;
        case 5:
            dev->attr_state.text_blink_level = 1;
            break;
        case 6:
            dev->attr_state.text_blink_level = 2;
            break;
        case 7:
            dev->attr_state.text_reversed = 1;
            break;
        case 8:
            dev->attr_state.text_blink_level = 3;
            break;
        case 9:
            dev->attr_state.text_strike = 1;
            break;
        case 22:
            dev->attr_state.text_bold = 0;
            dev->attr_state.text_dim = 0;
            break;
        case 23:
            dev->attr_state.text_italic = 0;
            break;
        case 24:
            dev->attr_state.text_underline = 0;
            break;
        case 25:
            dev->attr_state.text_blink_level = 0;
            break;
        case 27:
            dev->attr_state.text_reversed = 0;
            break;
        case 28:
            dev->attr_state.text_blink_level = 0;
            break;
        case 29:
            dev->attr_state.text_strike = 0;
            break;
        case 30: case 31: case 32: case 33:
        case 34: case 35: case 36: case 37:
            dev->attr_state.fg_color = palette[option - 30];
            break;
        case 38:
            if (dev->escape_seq_arg_count < 2) break;
            switch (dev->escape_seq_args[1]) {
                case 5:
                    if (dev->escape_seq_arg_count < 3) break;
                    dev->attr_state.fg_color = palette[dev->escape_seq_args[2]];
                    break;
                case 2:
                    if (dev->escape_seq_arg_count < 5) break;
                    uint8_t r = dev->escape_seq_args[2];
                    uint8_t g = dev->escape_seq_args[3];
                    uint8_t b = dev->escape_seq_args[4];
                    dev->attr_state.fg_color = (r << 16) | (g << 8) | b;
                    break;
                default:
                    break;
            }
            break;
        case 39:
            dev->attr_state.fg_color = palette[7];
            break;
        case 40: case 41: case 42: case 43:
        case 44: case 45: case 46: case 47:
            dev->attr_state.bg_color = palette[option - 40];
            break;
        case 48:
            if (dev->escape_seq_arg_count < 2) break;
            switch (dev->escape_seq_args[1]) {
                case 5:
                    if (dev->escape_seq_arg_count < 3) break;
                    dev->attr_state.bg_color = palette[dev->escape_seq_args[2]];
                    break;
                case 2:
                    if (dev->escape_seq_arg_count < 5) break;
                    uint8_t r = dev->escape_seq_args[2];
                    uint8_t g = dev->escape_seq_args[3];
                    uint8_t b = dev->escape_seq_args[4];
                    dev->attr_state.bg_color = (r << 16) | (g << 8) | b;
                    break;
                default:
                    break;
            }
            break;
        case 49:
            dev->attr_state.bg_color = palette[0];
            break;
        case 53:
            dev->attr_state.text_overlined = 1;
            break;
        case 55:
            dev->attr_state.text_overlined = 0;
            break;
        case 90: case 91: case 92: case 93:
        case 94: case 95: case 96: case 97:
            dev->attr_state.fg_color = palette[option - 90 + 8];
            break;
        case 100: case 101: case 102: case 103:
        case 104: case 105: case 106: case 107:
            dev->attr_state.bg_color = palette[option - 100 + 8];
            break;
    }
    dev->escape_seq_state = STATE_DEFAULT;
}

static void handle_dsr(struct ansi_term_device *dev)
{
    dev->escape_seq_state = STATE_DEFAULT;
}

static void handle_scosc(struct ansi_term_device *dev)
{
    int cursor_x, cursor_y;
    dev->condev_conif->get_cursor_pos(dev->condev, &cursor_x, &cursor_y);

    dev->saved_cursor_x = cursor_x;
    dev->saved_cursor_y = cursor_y;
    dev->escape_seq_state = STATE_DEFAULT;
}

static void handle_scorc(struct ansi_term_device *dev)
{
    dev->condev_conif->set_cursor_pos(dev->condev, dev->saved_cursor_x, dev->saved_cursor_y);
    dev->escape_seq_state = STATE_DEFAULT;
}

static void handle_csi(struct ansi_term_device *dev, char ch)
{
    switch (ch) {
        case 'A':
            handle_cuu(dev);
            break;
        case 'B':
            handle_cud(dev);
            break;
        case 'C':
            handle_cuf(dev);
            break;
        case 'D':
            handle_cub(dev);
            break;
        case 'E':
            handle_cnl(dev);
            break;
        case 'F':
            handle_cpl(dev);
            break;
        case 'G':
            handle_cha(dev);
            break;
        case 'f':
        case 'H':
            handle_cup_hvp(dev);
            break;
        case 'J':
            handle_ed(dev);
            break;
        case 'K':
            handle_el(dev);
            break;
        case 'S':
            handle_su(dev);
            break;
        case 'T':
            handle_sd(dev);
            break;
        case 'm':
            handle_sgr(dev);
            break;
        case 'n':
            handle_dsr(dev);
            break;
        case 's':
            handle_scosc(dev);
            break;
        case 'u':
            handle_scorc(dev);
            break;
        default:
            switch (ch) {
                case '0':  case '1':  case '2':  case '3':  case '4':
                case '5': case '6': case '7': case '8': case '9':
                    if (!dev->escape_seq_number_input) dev->escape_seq_arg_count++;
                    dev->escape_seq_number_input = 1;
                    dev->escape_seq_args[dev->escape_seq_arg_count - 1] *= 10;
                    dev->escape_seq_args[dev->escape_seq_arg_count - 1] += ch - '0';
                    break;
                case ';':
                case ':':
                    dev->escape_seq_number_input = 0;
                    break;
                default:
                    dev->escape_seq_state = STATE_DEFAULT;
                    break;
            }
            break;
    }
}

static void put_char(struct ansi_term_device *dev, char ch)
{
    if (dev->escape_seq_state != STATE_DEFAULT) {
        switch (dev->escape_seq_state) {
            case STATE_ESC:
                handle_esc(dev, ch);
                break;
            case STATE_CSI:
                handle_csi(dev, ch);
                break;
            default:
                dev->escape_seq_state = STATE_DEFAULT;
                break;
        }
        return;
    }

    int cursor_x, cursor_y, width, height;
    dev->condev_conif->get_cursor_pos(dev->condev, &cursor_x, &cursor_y);
    dev->condev_conif->get_dimension(dev->condev, &width, &height);

    switch (ch) {
        case '\a':
            break;
        case '\b':
            if (cursor_x < 1) break;
            cursor_x--;
            break;
        case '\t':
            cursor_x = ((cursor_x / 8) + 1) * 8;
            if (cursor_x < width) break;
            cursor_y++;
            if (cursor_y >= height) {
                cursor_y = height - 1;
                console_scroll(dev, 1, 0, 0, width - 1, height - 1);
            }
            cursor_x = 0;
            break;
        case '\n':
            cursor_x = 0;
            cursor_y++;
            if (cursor_y < height) break;
            cursor_y = height - 1;
            console_scroll(dev, 1, 0, 0, width - 1, height - 1);
            break;
        case '\v':
            cursor_y++;
            if (cursor_y < height) break;
            cursor_y = height - 1;
            console_scroll(dev, 1, 0, 0, width - 1, height - 1);
            break;
        case '\f':
            break;
        case '\r':
            cursor_x = 0;
            break;
        case '\033':
            dev->escape_seq_state = STATE_ESC;
            break;
        case 0x7F:
            if (cursor_x < 1) break;
            cursor_x--;
            console_erase(dev, cursor_x, cursor_y, cursor_x, cursor_y);
            break;
        default:
            if (ch < 0x20) break;
            console_putchar(dev, ch);
            cursor_x++;
            if (cursor_x < width) break;
            cursor_x = 0;
            cursor_y++;
            if (cursor_y < height) break;
            cursor_y = height - 1;
            console_scroll(dev, 1, 0, 0, width - 1, height - 1);
            break;
    }

    dev->condev_conif->set_cursor_pos(dev->condev, cursor_x, cursor_y);
    dev->condev_conif->flush(dev->condev);
}

static long write(struct device *_dev, const char *buf, unsigned long len)
{
    struct ansi_term_device *dev = (struct ansi_term_device *)_dev;

    for (int i = 0; i < len; i++) {
        put_char(dev, buf[i]);
    }

    return len;
}

static const struct char_interface charif = {
    .write = write,
};

static struct device *probe(const struct device_id *id);
static int remove(struct device *_dev);
static const void *get_interface(struct device *_dev, const char *name);

static struct device_driver drv = {
    .name = "ansiterm",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static struct device *probe(const struct device_id *id)
{
    struct ansi_term_device *dev = mm_allocate(sizeof(*dev));
    if (!dev) return NULL;
    dev->dev.driver = &drv;

    struct device_id *current_id = mm_allocate(sizeof(*current_id));
    current_id->type = DIT_STRING;
    current_id->string = "tty";
    dev->dev.id = current_id;

    if (id->type != DIT_STRING) return NULL;
    struct device *condev = find_device(id->string);
    if (!condev) return NULL;
    dev->condev = condev;
    
    const struct console_interface *condev_conif = condev->driver->get_interface(condev, "console");
    if (!condev_conif) return NULL;
    dev->condev_conif = condev_conif;

    dev->condev_conif->set_cursor_pos(dev->condev, 0, 0);
    
    dev->charif = &charif;
    
    dev->escape_seq_arg_count = 0;

    dev->saved_cursor_x = 0;
    dev->saved_cursor_y = 0;

    dev->attr_state.fg_color = 0xffffff;
    dev->attr_state.bg_color = 0x000000;
    dev->attr_state.text_blink_level = 0;
    dev->attr_state.text_bold = 0;
    dev->attr_state.text_dim = 0;
    dev->attr_state.text_italic = 0;
    dev->attr_state.text_underline = 0;
    dev->attr_state.text_strike = 0;
    dev->attr_state.text_overlined = 0;
    dev->attr_state.text_reversed = 0;

    register_device((struct device *)dev);

    return (struct device *)dev;
}

static int remove(struct device *_dev)
{
    struct ansi_term_device *dev = (struct ansi_term_device *)_dev;

    mm_free(dev);

    return 0;
}

static const void *get_interface(struct device *_dev, const char *name)
{
    struct ansi_term_device *dev = (struct ansi_term_device *)_dev;

    if (strcmp(name, "char") == 0) {
        return dev->charif;
    }

    return NULL;
}

__attribute__((constructor))
static void _register_driver(void)
{
    register_driver((struct device_driver *)&drv);
}
