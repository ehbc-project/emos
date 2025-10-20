#include <string.h>
#include <stdio.h>

#include <mm/mm.h>
#include <device/driver.h>
#include <interface/ps2.h>
#include <asm/isr.h>
#include <sys/io.h>
#include <asm/time.h>
#include <asm/pause.h>
#include <hid/hid.h>
#include <bus/bus.h>

struct i8042_data {
    struct resource *res[2];
};

struct key_event {
    uint16_t keycode;
    uint16_t flags;
};

static int wait_output_buffer_full(struct device *dev, unsigned int timeout)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    uint64_t tick_start = get_global_tick();

    while (io_in8(data->res[1]->base) & 0x01) {
        if (get_global_tick() - tick_start > timeout) return 1;
        _i686_pause();
    }

    return 0;
}

static int wait_output_buffer_empty(struct device *dev, unsigned int timeout)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    uint64_t tick_start = get_global_tick();

    while (!(io_in8(data->res[1]->base) & 0x01)) {
        if (get_global_tick() - tick_start > timeout) return 1;
        _i686_pause();
    }

    return 0;
}

static int wait_input_buffer_full(struct device *dev, unsigned int timeout)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    uint64_t tick_start = get_global_tick();

    while (io_in8(data->res[1]->base) & 0x02) {
        if (get_global_tick() - tick_start > timeout) return 1;
        _i686_pause();
    }

    return 0;
}

static void enable_port(struct device *dev, int port)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    io_out8(data->res[1]->base, !port ? 0xAE : 0xA8);
    io_out8(data->res[1]->base, 0x20);
    uint8_t prev_ccb = io_in8(data->res[0]->base);
    io_out8(data->res[1]->base, 0x60);
    io_out8(data->res[0]->base, prev_ccb | (!port ? 0x01 : 0x02));
}

static void disable_port(struct device *dev, int port)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    io_out8(data->res[1]->base, !port ? 0xAD : 0xA7);
    io_out8(data->res[1]->base, 0x20);
    uint8_t prev_ccb = io_in8(data->res[0]->base);
    io_out8(data->res[1]->base, 0x60);
    io_out8(data->res[0]->base, prev_ccb & ~(!port ? 0x01 : 0x02));
}

static int test_port(struct device *dev, int port)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    io_out8(data->res[1]->base, !port ? 0xAB : 0xA9);
    return io_in8(data->res[0]->base);
}

static int send_data(struct device *dev, int port, const uint8_t *buf, int len)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    int err;

    for (int i = 0; i < len; i++) {
        if (port) {
            io_out8(data->res[1]->base, 0xD4);
        }

        err = wait_input_buffer_full(dev, 1);
        if (err) return err;
        io_out8(data->res[0]->base, buf[i]);
    }

    return 0;
}

static int recv_data(struct device *dev, int port, uint8_t *buf, int len)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    int err;

    for (int i = 0; i < len; i++) {
        err = wait_output_buffer_empty(dev, 1);
        if (err) return err;
        buf[i] = io_in8(data->res[0]->base);
    }

    return 0;
}

static uint8_t irq_get_byte(struct device *dev)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    return io_in8(data->res[0]->base);
}

static const struct ps2_interface ps2if = {
    .enable_port = enable_port,
    .disable_port = disable_port,
    .test_port = test_port,
    .send_data = send_data,
    .recv_data = recv_data,
    .irq_get_byte = irq_get_byte,
};

static int probe(struct device *dev);
static int remove(struct device *dev);
static const void *get_interface(struct device *dev, const char *name);

static struct device_driver drv = {
    .name = "i8042",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static int probe(struct device *dev)
{
    struct resource *res[4];
    if (!dev->resource || !dev->resource->next || !dev->resource->next->next || !dev->resource->next->next->next) return 1;
    res[0] = dev->resource;
    res[1] = dev->resource->next;
    res[2] = dev->resource->next->next;
    res[3] = dev->resource->next->next->next;
    if (res[0]->type != RT_IOPORT || res[1]->type != RT_IOPORT || res[2]->type != RT_IRQ || res[3]->type != RT_IRQ) {
        return 1;
    }
    if (res[0]->limit != res[0]->base || res[1]->limit != res[1]->base || res[2]->limit != res[2]->base || res[3]->limit != res[3]->base) {
        return 1;
    }

    dev->name = "ps2c";
    dev->id = generate_device_id(dev->name);

    struct i8042_data *data = mm_allocate(sizeof(*data));
    data->res[0] = res[0];
    data->res[1] = res[1];

    dev->data = data;

    /* disable all devices */
    io_out8(res[1]->base, 0xAD);
    io_out8(res[1]->base, 0xA7);
    
    /* flush the output buffer*/
    io_in8(res[0]->base);

    /* disable translation & IRQ */
    io_out8(res[1]->base, 0x60);
    io_out8(res[0]->base, 0x00);

    /* perform controller self-test */
    io_out8(res[1]->base, 0xAA);
    wait_output_buffer_full(dev, 5);
    if (io_in8(res[0]->base) != 0x55) {
        return 1;
    }

    /* disable translation & IRQ (again) */
    io_out8(res[1]->base, 0x60);
    io_out8(res[0]->base, 0x00);

    /* determine if the second port is available */
    int has_second_port = 0;
    io_out8(res[1]->base, 0xA8);
    io_out8(res[1]->base, 0x20);
    if (!(io_in8(res[0]->base) & 0x20)) {
        has_second_port = 1;
        io_out8(res[1]->base, 0xA7);
    }

    /* initialize bus & child devices */
    struct bus *ps2_bus = mm_allocate_clear(1, sizeof(*ps2_bus));
    ps2_bus->dev = dev;
    ps2_bus->name = "ps2";
    register_bus(ps2_bus);

    {
        struct device *port1_device = mm_allocate_clear(1, sizeof(*port1_device));
        port1_device->bus = ps2_bus;
        port1_device->parent = dev;
        struct resource *nres = create_resource(NULL);
        nres->type = RT_BUS;
        nres->base = nres->limit = 0;
        nres->flags = 0;
        port1_device->resource = nres;
        nres = create_resource(nres);
        nres->type = res[2]->type;
        nres->base = res[2]->base;
        nres->limit = res[2]->limit;
        nres->flags = res[2]->flags;
        port1_device->driver = find_device_driver("ps2_keyboard");
        register_device(port1_device);
    }

    if (has_second_port) {
        struct device *port2_device = mm_allocate_clear(1, sizeof(*port2_device));
        port2_device->bus = ps2_bus;
        port2_device->parent = dev;
        struct resource *nres = create_resource(NULL);
        nres->type = RT_BUS;
        nres->base = nres->limit = 1;
        nres->flags = 0;
        port2_device->resource = nres;
        nres = create_resource(nres);
        nres->type = res[3]->type;
        nres->base = res[3]->base;
        nres->limit = res[3]->limit;
        nres->flags = res[3]->flags;
        port2_device->driver = find_device_driver("ps2_mouse");
        register_device(port2_device);
    }

    return 0;
}

static int remove(struct device *dev)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    mm_free(data);

    return 0;
}

static const void *get_interface(struct device *dev, const char *name)
{
    if (strcmp(name, "ps2") == 0) {
        return &ps2if;
    }

    return NULL;
}

DEVICE_DRIVER(drv)
