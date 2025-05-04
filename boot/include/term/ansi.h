#ifndef __TERM_ANSI_H__
#define __TERM_ANSI_H__

#include <stdint.h>

enum ansi_escape_state;

struct ansi_state {
    void *data;

    int cursor_x, cursor_y;
    int term_width, term_height;

    int escape_seq_args[8];
    int escape_seq_arg_count;
    int escape_seq_state;
    int escape_seq_number_input;

    uint32_t fg_color, bg_color;
    int saved_cursor_x, saved_cursor_y;

    uint32_t cursor_visible : 1;
    uint32_t text_blink_level : 2;
    uint32_t text_reversed : 1;
    uint32_t text_bold : 1;
    uint32_t text_dim : 1;
    uint32_t text_italic : 1;
    uint32_t text_underline : 1;
    uint32_t text_strike : 1;
    uint32_t text_overlined : 1;
    uint32_t : 22;
};

int ansi_init_state(void *data, int term_width, int term_height, struct ansi_state *state);

void ansi_putchar(struct ansi_state *state, char ch);

#endif
