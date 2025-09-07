#include <string.h>
#include <stdio.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <interface/char.h>
#include <interface/hid.h>
#include <interface/ps2.h>
#include <asm/isr.h>
#include <asm/io.h>
#include <asm/time.h>
#include <asm/pause.h>
#include <hid/hid.h>

enum sequence_state {
    SS_DEFAULT = 0,
    SS_EXTENDED,
    SS_BREAK,
    SS_BREAK_EXTENDED,
    SS_PAUSE_1,
    SS_PAUSE_2,
    SS_PAUSE_3,
    SS_PAUSE_4,
    SS_PAUSE_5,
    SS_PAUSE_6,
    SS_PAUSE_7,
    SS_CTRLPAUSE_1,
    SS_CTRLPAUSE_2,
    SS_CTRLPAUSE_3,
};

struct ps2_keyboard_data {
    struct device *ps2dev;
    const struct ps2_interface *ps2dev_ps2if;

    volatile int seqbuf_start, seqbuf_end;
    volatile uint8_t seqbuf[64];
    enum sequence_state seq_state;
    int scancode_set;
};

struct key_event {
    uint16_t keycode;
    uint16_t flags;
};

static const uint16_t translated_keycode_char_table[256] = {
    '\0', '\0', '\0', '\0', 'a' , 'b' , 'c' , 'd' ,  /* 0x00 - 0x07 */
    'e' , 'f' , 'g' , 'h' , 'i' , 'j' , 'k' , 'l' ,  /* 0x08 - 0x0F */
    'm' , 'n' , 'o' , 'p' , 'q' , 'r' , 's' , 't' ,  /* 0x10 - 0x17 */
    'u' , 'v' , 'w' , 'x' , 'y' , 'z' , '1' , '2' ,  /* 0x18 - 0x1F */
    '3' , '4' , '5' , '6' , '7' , '8' , '9' , '0' ,  /* 0x20 - 0x27 */
    '\n', '\0', '\b', '\t', ' ' , '-' , '=' , '[' ,  /* 0x28 - 0x2F */
    ']' , '\\', '#' , ':' , '\'', '`' , ',' , '.' ,  /* 0x30 - 0x37 */
    '/' , '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0x38 - 0x3F */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0x40 - 0x47 */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0x48 - 0x4F */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0x50 - 0x57 */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0x58 - 0x5F */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0x60 - 0x67 */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0x68 - 0x6F */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0x70 - 0x77 */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0x78 - 0x7F */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0x80 - 0x87 */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0x88 - 0x8F */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0x90 - 0x97 */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0x98 - 0x9F */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0xA0 - 0xA7 */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0xA8 - 0xAF */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0xB0 - 0xB7 */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0xB8 - 0xBF */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0xC0 - 0xC7 */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0xC8 - 0xCF */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0xD0 - 0xD7 */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0xD8 - 0xDF */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0xE0 - 0xE7 */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0xE8 - 0xEF */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0xF0 - 0xF7 */
    '\0', '\0', '\0', '\0', '\0', '\0', '\0', '\0',  /* 0xF8 - 0xFF */
};

static const uint16_t code_table_set1_noext_low[128] = {
    KEY_NONE,       KEY_ESC,        KEY_1,          KEY_2,
    KEY_3,          KEY_4,          KEY_5,          KEY_6,
    KEY_7,          KEY_8,          KEY_9,          KEY_0,
    KEY_MINUS,      KEY_EQUAL,      KEY_BACKSPACE,  KEY_TAB,
    KEY_Q,          KEY_W,          KEY_E,          KEY_R,
    KEY_T,          KEY_Y,          KEY_U,          KEY_I,
    KEY_O,          KEY_P,          KEY_LEFTBRACE,  KEY_RIGHTBRACE,
    KEY_ENTER,      KEY_LEFTCTRL,   KEY_A,          KEY_S,
    KEY_D,          KEY_F,          KEY_G,          KEY_H,
    KEY_J,          KEY_K,          KEY_L,          KEY_SEMICOLON,
    KEY_APOSTROPHE, KEY_GRAVE,      KEY_LEFTSHIFT,  KEY_BACKSLASH,
    KEY_Z,          KEY_X,          KEY_C,          KEY_V,
    KEY_B,          KEY_N,          KEY_M,          KEY_COMMA,
    KEY_DOT,        KEY_SLASH,      KEY_RIGHTSHIFT, KEY_KPASTERISK,
    KEY_LEFTALT,    KEY_SPACE,      KEY_CAPSLOCK,   KEY_F1,
    KEY_F2,         KEY_F3,         KEY_F4,         KEY_F5,
    KEY_F6,         KEY_F7,         KEY_F8,         KEY_F9,
    KEY_F10,        KEY_NUMLOCK,    KEY_SCROLLLOCK, KEY_KP7,
    KEY_KP8,        KEY_KP9,        KEY_KPMINUS,    KEY_KP4,
    KEY_KP5,        KEY_KP6,        KEY_KPPLUS,     KEY_KP1,
    KEY_KP2,        KEY_KP3,        KEY_KP0,        KEY_KPDOT,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_F11,
    KEY_F12,        KEY_KPEQUAL,    KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_F13,        KEY_F14,        KEY_F15,        KEY_F16,
    KEY_F17,        KEY_F18,        KEY_F19,        KEY_F20,
    KEY_F21,        KEY_F22,        KEY_F23,        KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_F24,        KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
};

static const uint16_t code_table_set1_ext_low[128] = { KEY_NONE, };

static const uint16_t code_table_set2_noext_low[132] = {
    KEY_ERR_OVF,    KEY_F9,         KEY_NONE,       KEY_F5,
    KEY_F3,         KEY_F1,         KEY_F2,         KEY_F12,
    KEY_F13,        KEY_F10,        KEY_F8,         KEY_F6,
    KEY_F4,         KEY_TAB,        KEY_GRAVE,      KEY_KPEQUAL,
    KEY_F14,        KEY_LEFTALT,    KEY_LEFTSHIFT,  KEY_NONE,
    KEY_LEFTCTRL,   KEY_Q,          KEY_1,          KEY_NONE,
    KEY_F15,        KEY_NONE,       KEY_Z,          KEY_S,
    KEY_A,          KEY_W,          KEY_2,          KEY_NONE,
    KEY_F16,        KEY_C,          KEY_X,          KEY_D,
    KEY_E,          KEY_4,          KEY_3,          KEY_NONE,
    KEY_F17,        KEY_SPACE,      KEY_V,          KEY_F,
    KEY_T,          KEY_R,          KEY_5,          KEY_NONE,
    KEY_F18,        KEY_N,          KEY_B,          KEY_H,
    KEY_G,          KEY_Y,          KEY_6,          KEY_NONE,
    KEY_F19,        KEY_NONE,       KEY_M,          KEY_J,
    KEY_U,          KEY_7,          KEY_8,          KEY_NONE,
    KEY_F20,        KEY_COMMA,      KEY_K,          KEY_I,
    KEY_O,          KEY_0,          KEY_9,          KEY_NONE,
    KEY_F21,        KEY_DOT,        KEY_SLASH,      KEY_L,
    KEY_SEMICOLON,  KEY_P,          KEY_MINUS,      KEY_NONE,
    KEY_F22,        KEY_NONE,       KEY_APOSTROPHE, KEY_NONE,
    KEY_LEFTBRACE,  KEY_EQUAL,      KEY_NONE,       KEY_F23,
    KEY_CAPSLOCK,   KEY_RIGHTSHIFT, KEY_ENTER,      KEY_RIGHTBRACE,
    KEY_NONE,       KEY_BACKSLASH,  KEY_NONE,       KEY_F24,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_BACKSPACE,  KEY_NONE,
    KEY_NONE,       KEY_KP1,        KEY_NONE,       KEY_KP4,
    KEY_KP7,        KEY_KPCOMMA,    KEY_NONE,       KEY_NONE,
    KEY_KP0,        KEY_KPDOT,      KEY_KP2,        KEY_KP5,
    KEY_KP6,        KEY_KP8,        KEY_ESC,        KEY_NUMLOCK,
    KEY_F11,        KEY_KPPLUS,     KEY_KP3,        KEY_KPMINUS,
    KEY_KPASTERISK, KEY_KP9,        KEY_SCROLLLOCK, KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_F7,
};

static const uint16_t code_table_set2_ext_low[128] = {
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_RIGHTALT,   KEY_NONE,       KEY_NONE,
    KEY_RIGHTCTRL,  KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_LEFTMETA,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_RIGHTMETA,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_POWER,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_MEDIA_SLEEP,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_KPENTER,    KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_MEDIA_COFFEE, KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_END,        KEY_NONE,       KEY_LEFT,
    KEY_HOME,       KEY_NONE,       KEY_NONE,       KEY_NONE,
    KEY_INSERT,     KEY_DELETE,     KEY_DOWN,       KEY_NONE,
    KEY_RIGHT,      KEY_UP,         KEY_NONE,       KEY_NONE,
    KEY_NONE,       KEY_NONE,       KEY_PAGEDOWN,   KEY_NONE,
    KEY_NONE,       KEY_PAGEUP,     KEY_NONE,       KEY_NONE,
};

static int translate_scancode_set1(struct device *dev, uint16_t *keycode, uint16_t *flags)
{
    struct ps2_keyboard_data *data = (struct ps2_keyboard_data *)dev->data;

    int ret = 1;
    uint16_t ret_keycode = KEY_NONE, ret_flags = 0;

    _i686_disable_interrupt();
    uint8_t byte = data->seqbuf[data->seqbuf_start];
    switch (byte) {
        case 0xE0:  /* extend */
            data->seq_state = SS_EXTENDED;
            break;
        case 0xE1:  /* pause */
            data->seq_state = SS_PAUSE_1;
            break;
        default:
            switch (data->seq_state) {
                case SS_DEFAULT:
                    if (byte == 0xF1) {
                        /* LANG2 */
                    } else if (byte == 0xF2) {
                        /* LANG1 */
                    } else if (byte == 0xFC) {
                        /* POST Fail */
                    } else if (byte < sizeof(code_table_set1_noext_low)) {
                        ret_keycode = code_table_set1_noext_low[byte];
                        ret = 0;
                    } else if (byte < 128 + sizeof(code_table_set1_noext_low)) {
                        ret_flags |= KEY_FLAG_BREAK;
                        ret_keycode = code_table_set1_noext_low[byte - 128];
                        ret = 0;
                    }
                    data->seq_state = SS_DEFAULT;
                    break;
                case SS_EXTENDED:
                    if (byte == 0x46) {
                        data->seq_state = SS_CTRLPAUSE_1;
                    } else if (byte < sizeof(code_table_set1_ext_low)) {
                        ret_keycode = code_table_set1_ext_low[byte];
                        ret = 0;
                        data->seq_state = SS_DEFAULT;
                    } else if (byte < 128 + sizeof(code_table_set1_ext_low)) {
                        ret_flags |= KEY_FLAG_BREAK;
                        ret_keycode = code_table_set1_ext_low[byte - 128];
                        ret = 0;
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_PAUSE_1:
                    if (byte == 0x1D) {
                        data->seq_state = SS_PAUSE_2;
                    } else {
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_PAUSE_2:
                    if (byte == 0x45) {
                        data->seq_state = SS_PAUSE_3;
                    } else {
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_PAUSE_3:
                    if (byte == 0xE1) {
                        data->seq_state = SS_PAUSE_4;
                    } else {
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_PAUSE_4:
                    if (byte == 0x9D) {
                        data->seq_state = SS_PAUSE_5;
                    } else {
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_PAUSE_5:
                    if (byte == 0xC5) {
                        ret_keycode = KEY_PAUSE;
                        ret = 0;
                    }
                    data->seq_state = SS_DEFAULT;
                    break;
                case SS_CTRLPAUSE_1:
                    if (byte == 0xE0) {
                        data->seq_state = SS_CTRLPAUSE_2;
                    } else {
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_CTRLPAUSE_2:
                    if (byte == 0xC6) {
                        ret_keycode = KEY_PAUSE;
                        ret_flags |= KEY_FLAG_BREAK;
                        ret = 0;
                    }
                    data->seq_state = SS_DEFAULT;
                    break;
                default:
                    data->seq_state = SS_DEFAULT;
                    break;
            }
            break;
    }

    data->seqbuf_start = (data->seqbuf_start + 1) % sizeof(data->seqbuf);
    _i686_enable_interrupt();

    if (!ret) {
        *keycode = ret_keycode;
        *flags = ret_flags;
    }

    return ret;
}

static int translate_scancode_set2(struct device *dev, uint16_t *keycode, uint16_t *flags)
{
    struct ps2_keyboard_data *data = (struct ps2_keyboard_data *)dev->data;

    int ret = 1;
    uint16_t ret_keycode = KEY_NONE, ret_flags = 0;

    _i686_disable_interrupt();
    uint8_t byte = data->seqbuf[data->seqbuf_start];
    switch (byte) {
        case 0xF0:  /* break */
            if (data->seq_state == SS_EXTENDED) {
                data->seq_state = SS_BREAK_EXTENDED;
            } else {
                data->seq_state = SS_BREAK;
            }
            break;
        case 0xE0:  /* extend */
            data->seq_state = SS_EXTENDED;
            break;
        case 0xE1:  /* pause */
            data->seq_state = SS_PAUSE_1;
            break;
        default:
            switch (data->seq_state) {
                case SS_BREAK:
                    ret_flags |= KEY_FLAG_BREAK;
                case SS_DEFAULT:
                    if (byte == 0xF1) {
                        /* LANG2 */
                    } else if (byte == 0xF2) {
                        /* LANG1 */
                    } else if (byte == 0xFC) {
                        /* POST Fail */
                    } else if (byte < sizeof(code_table_set2_noext_low)) {
                        ret_keycode = code_table_set2_noext_low[byte];
                        ret = 0;
                    }
                    data->seq_state = SS_DEFAULT;
                    break;
                case SS_BREAK_EXTENDED:
                    ret_flags |= KEY_FLAG_BREAK;
                case SS_EXTENDED:
                    if (byte == 0x7E) {
                        data->seq_state = SS_CTRLPAUSE_1;
                    } if (byte < sizeof(code_table_set2_ext_low)) {
                        ret_keycode = code_table_set2_ext_low[byte];
                        ret = 0;
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_PAUSE_1:
                    if (byte == 0x14) {
                        data->seq_state = SS_PAUSE_2;
                    } else {
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_PAUSE_2:
                    if (byte == 0x77) {
                        data->seq_state = SS_PAUSE_3;
                    } else {
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_PAUSE_3:
                    if (byte == 0xE1) {
                        data->seq_state = SS_PAUSE_4;
                    } else {
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_PAUSE_4:
                    if (byte == 0xF0) {
                        data->seq_state = SS_PAUSE_5;
                    } else {
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_PAUSE_5:
                    if (byte == 0x14) {
                        data->seq_state = SS_PAUSE_6;
                    } else {
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_PAUSE_6:
                    if (byte == 0xF0) {
                        data->seq_state = SS_PAUSE_7;
                    } else {
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_PAUSE_7:
                    if (byte == 0x77) {
                        ret_keycode = KEY_PAUSE;
                        ret = 0;
                    }
                    data->seq_state = SS_DEFAULT;
                    break;
                case SS_CTRLPAUSE_1:
                    if (byte == 0xE0) {
                        data->seq_state = SS_CTRLPAUSE_2;
                    } else {
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_CTRLPAUSE_2:
                    if (byte == 0xF0) {
                        data->seq_state = SS_CTRLPAUSE_3;
                    } else {
                        data->seq_state = SS_DEFAULT;
                    }
                    break;
                case SS_CTRLPAUSE_3:
                    if (byte == 0x7E) {
                        ret_keycode = KEY_PAUSE;
                        ret_flags |= KEY_FLAG_BREAK;
                        ret = 0;
                    }
                    data->seq_state = SS_DEFAULT;
                    break;
                default:
                    data->seq_state = SS_DEFAULT;
                    break;
            }
            break;
    }

    data->seqbuf_start = (data->seqbuf_start + 1) % sizeof(data->seqbuf);
    _i686_enable_interrupt();

    if (!ret) {
        *keycode = ret_keycode;
        *flags = ret_flags;
    }

    return ret;
}

static int translate_scancode(struct device *dev)
{
    struct ps2_keyboard_data *data = (struct ps2_keyboard_data *)dev->data;

    uint16_t translated, flags;
    char ch = 0;
    int ret;

    do {
        if (data->scancode_set == 1) {
            ret = translate_scancode_set1(dev, &translated, &flags);
        } else if (data->scancode_set == 2) {
            ret = translate_scancode_set2(dev, &translated, &flags);
        }
    } while (ret);

    if (translated < sizeof(translated_keycode_char_table)) {
        ch = translated_keycode_char_table[translated];
    }

    return (flags & KEY_FLAG_BREAK) ? 0 : ch;
}

static long read(struct device *dev, char *buf, long len)
{
    struct ps2_keyboard_data *data = (struct ps2_keyboard_data *)dev->data;

    int read_len = 0;

    while (read_len < len) {
        while (data->seqbuf_start == data->seqbuf_end) {
            _i686_pause();
        }
        int ch = translate_scancode(dev);
        if (ch) {
            *buf++ = ch;
            read_len++;
        }
    }

    return read_len;
}

static const struct char_interface charif = {
    .read = read,
};

static int wait_event(struct device *dev)
{
    struct ps2_keyboard_data *data = (struct ps2_keyboard_data *)dev->data;

    while (data->seqbuf_start == data->seqbuf_end) {
        _i686_pause();
    }

    return 0;
}

static int poll_event(struct device *dev, uint16_t *key, uint16_t *flags)
{
    struct ps2_keyboard_data *data = (struct ps2_keyboard_data *)dev->data;

    if (data->scancode_set == 1) {
        return translate_scancode_set1(dev, key, flags);
    } else if (data->scancode_set == 2) {
        return translate_scancode_set2(dev, key, flags);
    }

    return 1;
}

static const struct hid_interface hidif = {
    .wait_event = wait_event,
    .poll_event = poll_event,
};

static void keyboard_isr(struct device *dev, int num)
{
    struct ps2_keyboard_data *data = (struct ps2_keyboard_data *)dev->data;

    data->seqbuf[data->seqbuf_end] = data->ps2dev_ps2if->irq_get_byte(data->ps2dev);
    int next_seqbuf_end = (data->seqbuf_end + 1) % sizeof(data->seqbuf);
    if (next_seqbuf_end == data->seqbuf_start) {
        return;
    }
    data->seqbuf_end = next_seqbuf_end;
}

static int probe(struct device *dev);
static int remove(struct device *dev);
static const void *get_interface(struct device *dev, const char *name);

static struct device_driver drv = {
    .name = "ps2_keyboard",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static int probe(struct device *dev)
{
    if (!dev->resource || !dev->resource->next) return 1;
    if (dev->resource->type != RT_BUS || dev->resource->next->type != RT_IRQ) return 1;
    if (dev->resource->limit != dev->resource->base || dev->resource->next->limit != dev->resource->next->base) return 1;

    struct device *ps2dev = dev->parent;
    if (!ps2dev) return 1;
    const struct ps2_interface *ps2dev_ps2if = ps2dev->driver->get_interface(ps2dev, "ps2");
    if (!ps2dev_ps2if) return 1;

    dev->name = "kbd";
    dev->id = generate_device_id(dev->name);

    struct ps2_keyboard_data *data = mm_allocate(sizeof(*data));
    data->ps2dev = ps2dev;
    data->ps2dev_ps2if = ps2dev_ps2if;
    data->seqbuf_start = data->seqbuf_end = 0;
    data->seq_state = SS_DEFAULT;
    data->scancode_set = 1;

    dev->data = data;

    _pc_set_interrupt_handler(dev->resource->next->base, dev, keyboard_isr);
    
    _i686_disable_interrupt();
    
    int err = ps2dev_ps2if->test_port(ps2dev, dev->resource->base);
    if (err) {
        return err;
    }

    ps2dev_ps2if->enable_port(ps2dev, dev->resource->base);

    /* reset device */
    uint8_t buf[2] = { 0xFF };
    ps2dev_ps2if->send_data(ps2dev, dev->resource->base, buf, 1);
    ps2dev_ps2if->recv_data(ps2dev, dev->resource->base, buf, 1);
    if (buf[0] != 0xFA) {
        return 1;
    }

    /* try scan code set 2 */
    buf[0] = 0xF0;
    buf[1] = 0x02;
    ps2dev_ps2if->send_data(ps2dev, dev->resource->base, buf, 2);
    ps2dev_ps2if->recv_data(ps2dev, dev->resource->base, buf, 1);
    if (buf[0] == 0xFA) {
        data->scancode_set = 2;
    }

    _i686_enable_interrupt();

    return 0;
}

static int remove(struct device *dev)
{
    struct ps2_keyboard_data *data = (struct ps2_keyboard_data *)dev->data;

    mm_free(data);

    return 0;
}

static const void *get_interface(struct device *dev, const char *name)
{
    if (strcmp(name, "char") == 0) {
        return &charif;
    } else if (strcmp(name, "hid") == 0) {
        return &hidif;
    }

    return NULL;
}

__attribute__((constructor))
static void _register_driver(void)
{
    register_device_driver(&drv);
}
