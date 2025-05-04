#include "term/ansi.h"

#include <stdint.h>

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

void term_erase(struct ansi_state *state, int x0, int y0, int x1, int y1);
void term_putchar(struct ansi_state *state, char ch);
void term_scroll(struct ansi_state *state, int n, int x0, int y0, int x1, int y1);
void term_draw_cursor(struct ansi_state *state);

int ansi_init_state(void *data, int term_width, int term_height, struct ansi_state *state)
{
    state->data = data;

    state->cursor_x = 0;
    state->cursor_y = 0;

    state->term_width = term_width;
    state->term_height = term_height;

    state->escape_seq_arg_count = 0;

    state->fg_color = 0xffffff;
    state->bg_color = 0x000000;

    state->saved_cursor_x = 0;
    state->saved_cursor_y = 0;

    state->cursor_visible = 1;
    state->text_blink_level = 0;
    state->text_bold = 0;
    state->text_dim = 0;
    state->text_italic = 0;
    state->text_underline = 0;
    state->text_strike = 0;
    state->text_overlined = 0;
    return 0;
}

static void handle_esc(struct ansi_state *state, char ch)
{
    for (int i = 0; i < 8; i++) {
        state->escape_seq_args[i] = 0;
    }
    state->escape_seq_arg_count = 0;
    state->escape_seq_number_input = 0;
    switch (ch) {
        case 'N':
            state->escape_seq_state = STATE_SS2;
            break;
        case 'O':
            state->escape_seq_state = STATE_SS3;
            break;
        case 'P':
            state->escape_seq_state = STATE_DCS;
            break;
        case '[':
            state->escape_seq_state = STATE_CSI;
            break;
        case '\\':
            state->escape_seq_state = STATE_ST;
            break;
        case ']':
            state->escape_seq_state = STATE_OSC;
            break;
        case 'X':
            state->escape_seq_state = STATE_SOS;
            break;
        case '^':
            state->escape_seq_state = STATE_PM;
            break;
        case '_':
            state->escape_seq_state = STATE_APC;
            break;
        default:
            state->escape_seq_state = STATE_DEFAULT;
            break;
    }
}

static void handle_cuu(struct ansi_state *state)
{
    int count = (state->escape_seq_arg_count < 1) ? 1 : state->escape_seq_args[0];
    count = (count < state->cursor_y) ? count : state->cursor_y;
    state->cursor_y -= count;
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_cud(struct ansi_state *state)
{
    int count = (state->escape_seq_arg_count < 1) ? 1 : state->escape_seq_args[0];
    count = (count < state->term_height - state->cursor_y) ? count : state->term_height - state->cursor_y;
    state->cursor_y += count;
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_cuf(struct ansi_state *state)
{
    int count = (state->escape_seq_arg_count < 1) ? 1 : state->escape_seq_args[0];
    count = (count < state->term_width - state->cursor_x) ? count : state->term_width - state->cursor_x;
    state->cursor_x += count;
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_cub(struct ansi_state *state)
{
    int count = (state->escape_seq_arg_count < 1) ? 1 : state->escape_seq_args[0];
    count = (count < state->cursor_x) ? count : state->cursor_x;
    state->cursor_x -= count;
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_cnl(struct ansi_state *state)
{
    int count = (state->escape_seq_arg_count < 1) ? 1 : state->escape_seq_args[0];
    count = (count < state->term_height - state->cursor_y) ? count : state->term_height - state->cursor_y;
    state->cursor_y += count;
    state->cursor_x = 0;
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_cpl(struct ansi_state *state)
{
    int count = (state->escape_seq_arg_count < 1) ? 1 : state->escape_seq_args[0];
    count = (count < state->cursor_y) ? count : state->cursor_y;
    state->cursor_y -= count;
    state->cursor_x = 0;
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_cha(struct ansi_state *state)
{
    int pos = (state->escape_seq_arg_count < 1) ? 1 : state->escape_seq_args[0];
    state->cursor_x = pos - 1;
    if (state->cursor_x < 0) {
        state->cursor_x = 0;
    }
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_cup_hvp(struct ansi_state *state)
{
    int ypos = (state->escape_seq_arg_count < 1) ? 1 : state->escape_seq_args[0];
    int xpos = (state->escape_seq_arg_count < 2) ? 1 : state->escape_seq_args[1];
    state->cursor_x = xpos - 1;
    if (state->cursor_x < 0) {
        state->cursor_x = 0;
    }
    state->cursor_y = ypos - 1;
    if (state->cursor_y < 0) {
        state->cursor_y = 0;
    }
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_ed(struct ansi_state *state)
{
    int option = (state->escape_seq_arg_count < 1) ? 0 : state->escape_seq_args[0];
    switch (option) {
        case 0:
            term_erase(state, state->cursor_x, state->cursor_y, state->term_width - 1, state->cursor_y);
            if (state->cursor_y < state->term_height - 1) break;
            term_erase(state, 0, state->cursor_y + 1, state->term_width - 1, state->term_height - 1);
            break;
        case 1:
            term_erase(state, 0, state->cursor_y, state->cursor_x, state->cursor_y);
            if (state->cursor_x > 0) break;
            term_erase(state, 0, 0, state->term_width - 1, state->cursor_y - 1);
            break;
        case 2:
        case 3:
            term_erase(state, 0, 0, state->term_width - 1, state->term_height - 1);
            break;
    }
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_el(struct ansi_state *state)
{
    int option = (state->escape_seq_arg_count < 1) ? 0 : state->escape_seq_args[0];
    switch (option) {
        case 0:
            term_erase(state, state->cursor_x, state->cursor_y, state->term_width - 1, state->cursor_y);
            break;
        case 1:
            term_erase(state, 0, state->cursor_y, state->cursor_x, state->cursor_y);
            break;
    }
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_su(struct ansi_state *state)
{
    int amount = (state->escape_seq_arg_count < 1) ? 1 : state->escape_seq_args[0];
    term_scroll(state, amount, 0, 0, state->term_width - 1, state->term_height - 1);
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_sd(struct ansi_state *state)
{
    int amount = (state->escape_seq_arg_count < 1) ? 1 : state->escape_seq_args[0];
    term_scroll(state, -amount, 0, 0, state->term_width - 1, state->term_height - 1);
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_sgr(struct ansi_state *state)
{
    int option = (state->escape_seq_arg_count < 1) ? 0 : state->escape_seq_args[0];
    switch (option) {
        case 0:
            state->bg_color = palette[0];
            state->fg_color = palette[15];
            state->text_blink_level = 0;
            state->text_bold = 0;
            state->text_dim = 0;
            state->text_italic = 0;
            state->text_underline = 0;
            state->text_strike = 0;
            state->text_overlined = 0;
            break;
        case 1:
            state->text_bold = 1;
            break;
        case 2:
            state->text_dim = 1;
            break;
        case 3:
            state->text_italic = 1;
            break;
        case 4:
            state->text_underline = 1;
            break;
        case 5:
            state->text_blink_level = 1;
            break;
        case 6:
            state->text_blink_level = 2;
            break;
        case 7:
            state->text_reversed = 1;
            break;
        case 8:
            state->text_blink_level = 3;
            break;
        case 9:
            state->text_strike = 1;
            break;
        case 22:
            state->text_bold = 0;
            state->text_dim = 0;
            break;
        case 23:
            state->text_italic = 0;
            break;
        case 24:
            state->text_underline = 0;
            break;
        case 25:
            state->text_blink_level = 0;
            break;
        case 27:
            state->text_reversed = 0;
            break;
        case 28:
            state->text_blink_level = 0;
            break;
        case 29:
            state->text_strike = 0;
            break;
        case 30: case 31: case 32: case 33:
        case 34: case 35: case 36: case 37:
            state->fg_color = palette[option - 30];
            break;
        case 38:
            if (state->escape_seq_arg_count < 2) break;
            switch (state->escape_seq_args[1]) {
                case 5:
                    if (state->escape_seq_arg_count < 3) break;
                    state->fg_color = palette[state->escape_seq_args[2]];
                    break;
                case 2:
                    if (state->escape_seq_arg_count < 5) break;
                    uint8_t r = state->escape_seq_args[2];
                    uint8_t g = state->escape_seq_args[3];
                    uint8_t b = state->escape_seq_args[4];
                    state->fg_color = (r << 16) | (g << 8) | b;
                    break;
                default:
                    break;
            }
            break;
        case 39:
            state->fg_color = palette[7];
            break;
        case 40: case 41: case 42: case 43:
        case 44: case 45: case 46: case 47:
            state->bg_color = palette[option - 40];
            break;
        case 48:
            if (state->escape_seq_arg_count < 2) break;
            switch (state->escape_seq_args[1]) {
                case 5:
                    if (state->escape_seq_arg_count < 3) break;
                    state->bg_color = palette[state->escape_seq_args[2]];
                    break;
                case 2:
                    if (state->escape_seq_arg_count < 5) break;
                    uint8_t r = state->escape_seq_args[2];
                    uint8_t g = state->escape_seq_args[3];
                    uint8_t b = state->escape_seq_args[4];
                    state->bg_color = (r << 16) | (g << 8) | b;
                    break;
                default:
                    break;
            }
            break;
        case 49:
            state->bg_color = palette[0];
            break;
        case 53:
            state->text_overlined = 1;
            break;
        case 55:
            state->text_overlined = 0;
            break;
        case 90: case 91: case 92: case 93:
        case 94: case 95: case 96: case 97:
            state->fg_color = palette[option - 90 + 8];
            break;
        case 100: case 101: case 102: case 103:
        case 104: case 105: case 106: case 107:
            state->bg_color = palette[option - 100 + 8];
            break;
    }
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_dsr(struct ansi_state *state)
{
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_scosc(struct ansi_state *state)
{
    state->saved_cursor_x = state->cursor_x;
    state->saved_cursor_y = state->cursor_y;
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_scorc(struct ansi_state *state)
{
    state->cursor_x = state->saved_cursor_x;
    state->cursor_y = state->saved_cursor_y;
    state->escape_seq_state = STATE_DEFAULT;
}

static void handle_csi(struct ansi_state *state, char ch)
{
    switch (ch) {
        case 'A':
            handle_cuu(state);
            break;
        case 'B':
            handle_cud(state);
            break;
        case 'C':
            handle_cuf(state);
            break;
        case 'D':
            handle_cub(state);
            break;
        case 'E':
            handle_cnl(state);
            break;
        case 'F':
            handle_cpl(state);
            break;
        case 'G':
            handle_cha(state);
            break;
        case 'f':
        case 'H':
            handle_cup_hvp(state);
            break;
        case 'J':
            handle_ed(state);
            break;
        case 'K':
            handle_el(state);
            break;
        case 'S':
            handle_su(state);
            break;
        case 'T':
            handle_sd(state);
            break;
        case 'm':
            handle_sgr(state);
            break;
        case 'n':
            handle_dsr(state);
            break;
        case 's':
            handle_scosc(state);
            break;
        case 'u':
            handle_scorc(state);
            break;
        default:
            switch (ch) {
                case '0':  case '1':  case '2':  case '3':  case '4':
                case '5': case '6': case '7': case '8': case '9':
                    if (!state->escape_seq_number_input) state->escape_seq_arg_count++;
                    state->escape_seq_number_input = 1;
                    state->escape_seq_args[state->escape_seq_arg_count - 1] *= 10;
                    state->escape_seq_args[state->escape_seq_arg_count - 1] += ch - '0';
                    break;
                case ';':
                case ':':
                    state->escape_seq_number_input = 0;
                    break;
                default:
                    state->escape_seq_state = STATE_DEFAULT;
                    break;
            }
            break;
    }
}

void ansi_putchar(struct ansi_state *state, char ch)
{
    if (state->escape_seq_state != STATE_DEFAULT) {
        switch (state->escape_seq_state) {
            case STATE_ESC:
                handle_esc(state, ch);
                break;
            case STATE_CSI:
                handle_csi(state, ch);
                break;
            default:
                state->escape_seq_state = STATE_DEFAULT;
                break;
        }
        return;
    }

    switch (ch) {
        case '\a':
            break;
        case '\b':
            if (state->cursor_x < 1) break;
            state->cursor_x--;
            break;
        case '\t':
            state->cursor_x += 8;
            if (state->cursor_x < state->term_width) break;
            state->cursor_y++;
            if (state->cursor_y >= state->term_height) {
                state->cursor_y = state->term_height - 1;
                term_scroll(state, 1, 0, 0, state->term_width - 1, state->term_height - 1);
            }
            state->cursor_x = 0;
            break;
        case '\n':
            state->cursor_x = 0;
            state->cursor_y++;
            if (state->cursor_y < state->term_height) break;
            state->cursor_y = state->term_height - 1;
            term_scroll(state, 1, 0, 0, state->term_width - 1, state->term_height - 1);
            break;
        case '\v':
            state->cursor_y++;
            if (state->cursor_y < state->term_height) break;
            state->cursor_y = state->term_height - 1;
            term_scroll(state, 1, 0, 0, state->term_width - 1, state->term_height - 1);
            break;
        case '\f':
            break;
        case '\r':
            state->cursor_x = 0;
            break;
        case '\e':
            state->escape_seq_state = STATE_ESC;
            break;
        case 0x7F:
            if (state->cursor_x < 1) break;
            state->cursor_x--;
            term_erase(state, state->cursor_x, state->cursor_y, state->cursor_x, state->cursor_y);
            break;
        default:
            if (ch < 0x20) break;
            term_putchar(state, ch);
            state->cursor_x++;
            if (state->cursor_x < state->term_width) break;
            state->cursor_x = 0;
            state->cursor_y++;
            if (state->cursor_y < state->term_height) break;
            state->cursor_y = state->term_height - 1;
            term_scroll(state, 1, 0, 0, state->term_width - 1, state->term_height - 1);
            break;
    }
    // term_draw_cursor(state);
}
