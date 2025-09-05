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
    SS_XMOVEMENT,
    SS_YMOVEMENT,
    SS_ZMOVEMENT,
};

struct ps2_keyboard_data {
    struct device *ps2dev;
    const struct ps2_interface *ps2dev_ps2if;

    volatile int seqbuf_start, seqbuf_end;
    volatile uint8_t seqbuf[64];
    enum sequence_state seq_state;
    uint8_t prev_button_state, byte0;
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

    int ret = 1, advance = 0;
    uint16_t ret_key = 0, ret_flags = 0;

    _i686_disable_interrupt();
    uint8_t byte = data->seqbuf[data->seqbuf_start];
    switch (data->seq_state) {
        case SS_DEFAULT:
            if ((byte & 0x08) && !(byte & 0xC0)) {
                if ((byte & 0x04) != (data->prev_button_state & 0x04)) {
                    ret_key = KEY_MOUSEBTNM;
                    ret_flags = (byte & 0x04) ? 0 : KEY_FLAG_BREAK;
                    data->prev_button_state = (data->prev_button_state & ~0x04) | (byte & 0x04);
                    ret = 0;
                } else if ((byte & 0x02) != (data->prev_button_state & 0x02)) {
                    ret_key = KEY_MOUSEBTNR;
                    ret_flags = (byte & 0x02) ? 0 : KEY_FLAG_BREAK;
                    data->prev_button_state = (data->prev_button_state & ~0x02) | (byte & 0x02);
                    ret = 0;
                } else if ((byte & 0x01) != (data->prev_button_state & 0x01)) {
                    ret_key = KEY_MOUSEBTNL;
                    ret_flags = (byte & 0x01) ? 0 : KEY_FLAG_BREAK;
                    data->prev_button_state = (data->prev_button_state & ~0x01) | (byte & 0x01);
                    ret = 0;
                } else {
                    data->byte0 = byte;
                    data->seq_state = SS_XMOVEMENT;
                    advance = 1;
                }
            } else {
                advance = 1;
            }
            break;
        case SS_XMOVEMENT:
            if (data->byte0 & 0x40) break;
            if (data->byte0 & 0x10) {
                ret_key = 0x100 - byte;
                ret_flags = KEY_FLAG_XMOVE | KEY_FLAG_NEGATIVE;
            } else {
                ret_key = byte;
                ret_flags = KEY_FLAG_XMOVE;
            }
            data->seq_state = SS_YMOVEMENT;
            advance = 1;
            ret = 0;
            break;
        case SS_YMOVEMENT:
            if (data->byte0 & 0x80) break;
            ret_key = byte;
            if (data->byte0 & 0x20) {
                ret_key = 0x100 - byte;
                ret_flags = KEY_FLAG_YMOVE | KEY_FLAG_NEGATIVE;
            } else {
                ret_key = byte;
                ret_flags = KEY_FLAG_YMOVE;
            }
            data->seq_state = SS_DEFAULT;
            advance = 1;
            ret = 0;
            break;
        default:
            break;
    }

    data->seqbuf_start = (data->seqbuf_start + advance) % sizeof(data->seqbuf);
    _i686_enable_interrupt();

    if (!ret) {
        *key = ret_key;
        *flags = ret_flags;
    }

    return ret;
}

static const struct hid_interface hidif = {
    .wait_event = wait_event,
    .poll_event = poll_event,
};

static void mouse_isr(struct device *dev, int num)
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
    .name = "ps2_mouse",
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

    dev->name = "mouse";
    dev->id = generate_device_id(dev->name);

    struct ps2_keyboard_data *data = mm_allocate(sizeof(*data));
    data->ps2dev = ps2dev;
    data->ps2dev_ps2if = ps2dev_ps2if;
    data->seqbuf_start = data->seqbuf_end = 0;
    data->seq_state = SS_DEFAULT;

    dev->data = data;
    
    _i686_disable_interrupt();

    _pc_set_interrupt_handler(dev->resource->next->base, dev, mouse_isr);

    int err = ps2dev_ps2if->test_port(ps2dev, dev->resource->base);
    if (err) {
        return err;
    }

    ps2dev_ps2if->enable_port(ps2dev, dev->resource->base);

    /* reset device */
    uint8_t buf[2] = { 0xFF };
    ps2dev_ps2if->send_data(ps2dev, dev->resource->base, buf, 1);
    ps2dev_ps2if->recv_data(ps2dev, dev->resource->base, buf, 1);

    /* set scaling 2:1 */
    buf[0] = 0xE7;
    ps2dev_ps2if->send_data(ps2dev, dev->resource->base, buf, 1);
    ps2dev_ps2if->recv_data(ps2dev, dev->resource->base, buf, 1);

    /* set sample rate */
    buf[0] = 0xF3;
    buf[1] = 40;
    ps2dev_ps2if->send_data(ps2dev, dev->resource->base, buf, 2);
    ps2dev_ps2if->recv_data(ps2dev, dev->resource->base, buf, 1);

    /* enable data reporting */
    buf[0] = 0xF4;
    ps2dev_ps2if->send_data(ps2dev, dev->resource->base, buf, 1);
    ps2dev_ps2if->recv_data(ps2dev, dev->resource->base, buf, 1);

    _i686_enable_interrupt();

    return 0;
}

static int remove(struct device *dev)
{
    return 0;
}

static const void *get_interface(struct device *dev, const char *name)
{
    if (strcmp(name, "hid") == 0) {
        return &hidif;
    }

    return NULL;
}

__attribute__((constructor))
static void _register_driver(void)
{
    register_device_driver(&drv);
}
