#ifndef __INTERFACE_CONSOLE_H__
#define __INTERFACE_CONSOLE_H__

#include <stdint.h>

#include <device/device.h>

struct console_char_attributes {
    uint32_t fg_color : 24;
    uint32_t text_blink_level : 2;
    uint32_t text_reversed : 1;
    uint32_t text_bold : 1;
    uint32_t text_dim : 1;
    uint32_t text_italic : 1;
    uint32_t text_underline : 1;
    uint32_t text_strike : 1;
    
    uint32_t bg_color : 24;
    uint32_t text_overlined : 1;
    uint32_t : 7;
};

struct console_char_cell {
    struct console_char_attributes attr;
    uint32_t codepoint;
};

struct console_interface {
    void (*set_dimension)(struct device *, int, int);
    void (*get_dimension)(struct device *, int *, int *);
    struct console_char_cell *(*get_buffer)(struct device *);
    void (*invalidate)(struct device *, int, int, int, int);
    void (*flush)(struct device *);
    void (*present)(struct device *);
    void (*set_cursor_pos)(struct device *, int, int);
    void (*get_cursor_pos)(struct device *, int *, int *);
    void (*set_cursor_attr)(struct device *, const void *);
    void (*get_cursor_attr)(struct device *, void *);
};

#endif // __INTERFACE_CONSOLE_H__
