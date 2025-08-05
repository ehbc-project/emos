#ifndef __DEVICE_VIRTUAL_ANSI_TERM_H__
#define __DEVICE_VIRTUAL_ANSI_TERM_H__

#include <stdint.h>

#include <device/device.h>
#include <device/video/vconsole.h>
#include <interface/char.h>
#include <interface/console.h>

struct ansi_term_device {
    struct device dev;

    struct device *condev;
    const struct console_interface *condev_conif;

    int escape_seq_args[8];
    int escape_seq_arg_count;
    int escape_seq_state;
    int escape_seq_number_input;
    int saved_cursor_x, saved_cursor_y;
    struct console_char_attributes attr_state;

    const struct char_interface *charif;
};

#endif  // __DEVICE_VIRTUAL_ANSI_TERM_H__
