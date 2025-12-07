#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <eboot/asm/isr.h>
#include <eboot/asm/io.h>
#include <eboot/asm/time.h>
#include <eboot/asm/intrinsics/misc.h>

#include <eboot/status.h>
#include <eboot/macros.h>
#include <eboot/hid.h>
#include <eboot/device.h>
#include <eboot/interface/ps2.h>

struct i8042_data {
    uint16_t io_data, io_ctrl;
    int irq_port0, irq_port1;
    struct resource *res[2];
};

struct key_event {
    uint16_t keycode;
    uint16_t flags;
};

static status_t wait_output_buffer_full(struct device *dev, unsigned int timeout)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    uint64_t tick_start = get_global_tick();

    while (io_in8(data->io_ctrl) & 0x01) {
        if (get_global_tick() - tick_start > timeout) return STATUS_IO_TIMEOUT;
        _i686_pause();
    }

    return STATUS_SUCCESS;
}

static status_t wait_output_buffer_empty(struct device *dev, unsigned int timeout)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    uint64_t tick_start = get_global_tick();

    while (!(io_in8(data->io_ctrl) & 0x01)) {
        if (get_global_tick() - tick_start > timeout) return STATUS_IO_TIMEOUT;
        _i686_pause();
    }

    return STATUS_SUCCESS;
}

static status_t wait_input_buffer_full(struct device *dev, unsigned int timeout)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    uint64_t tick_start = get_global_tick();

    while (io_in8(data->io_ctrl) & 0x02) {
        if (get_global_tick() - tick_start > timeout) return STATUS_IO_TIMEOUT;
        _i686_pause();
    }

    return STATUS_SUCCESS;
}

static status_t enable_port(struct device *dev, int port)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    io_out8(data->io_ctrl, !port ? 0xAE : 0xA8);
    io_out8(data->io_ctrl, 0x20);
    uint8_t prev_ccb = io_in8(data->io_data);
    io_out8(data->io_ctrl, 0x60);
    io_out8(data->io_data, prev_ccb | (!port ? 0x01 : 0x02));

    return STATUS_SUCCESS;
}

static status_t disable_port(struct device *dev, int port)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    io_out8(data->io_ctrl, !port ? 0xAD : 0xA7);
    io_out8(data->io_ctrl, 0x20);
    uint8_t prev_ccb = io_in8(data->io_data);
    io_out8(data->io_ctrl, 0x60);
    io_out8(data->io_data, prev_ccb & ~(!port ? 0x01 : 0x02));

    return STATUS_SUCCESS;
}

static status_t test_port(struct device *dev, int port)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    io_out8(data->io_ctrl, !port ? 0xAB : 0xA9);
    return io_in8(data->io_data) ? STATUS_HARDWARE_NOT_FOUND : STATUS_SUCCESS;
}

static status_t send_data(struct device *dev, int port, const uint8_t *buf, int len)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;
    status_t status;

    for (int i = 0; i < len; i++) {
        if (port) {
            io_out8(data->io_ctrl, 0xD4);
        }

        status = wait_input_buffer_full(dev, 1);
        if (!CHECK_SUCCESS(status)) return status;

        io_out8(data->io_data, buf[i]);
    }

    return STATUS_SUCCESS;
}

static status_t recv_data(struct device *dev, int port, uint8_t *buf, int len)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;
    status_t status;

    for (int i = 0; i < len; i++) {
        status = wait_output_buffer_empty(dev, 1);
        if (!CHECK_SUCCESS(status)) return status;

        buf[i] = io_in8(data->io_data);
    }

    return STATUS_SUCCESS;
}

static status_t irq_get_byte(struct device *dev, uint8_t *result)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    uint8_t byte = io_in8(data->io_data);

    if (result) *result = byte;

    return STATUS_SUCCESS;
}

static const struct ps2_interface ps2if = {
    .enable_port = enable_port,
    .disable_port = disable_port,
    .test_port = test_port,
    .send_data = send_data,
    .recv_data = recv_data,
    .irq_get_byte = irq_get_byte,
};

static status_t probe(struct device **devout, struct device_driver *drv, struct device *parent, struct resource *rsrc, int rsrc_cnt);
static status_t remove(struct device *dev);
static status_t get_interface(struct device *dev, const char *name, const void **result);

static void i8042_init(void)
{
    status_t status;
    struct device_driver *drv;

    status = device_driver_create(&drv);
    if (!CHECK_SUCCESS(status)) {
        panic("cannot register device driver \"i8042\"");
    }

    drv->name = "i8042";
    drv->probe = probe;
    drv->remove = remove;
    drv->get_interface = get_interface;
}

static status_t probe(struct device **devout, struct device_driver *drv, struct device *parent, struct resource *rsrc, int rsrc_cnt)
{
    status_t status;
    struct device *dev = NULL;
    struct i8042_data *data = NULL;
    struct device *idev = NULL;
    struct device_driver *kbdrv = NULL;
    struct device_driver *msdrv = NULL;

    if (!rsrc || rsrc_cnt != 4 ||
        rsrc[0].type != RT_IOPORT || rsrc[0].base != rsrc[0].limit ||
        rsrc[1].type != RT_IOPORT || rsrc[1].base != rsrc[1].limit ||
        rsrc[2].type != RT_IRQ || rsrc[2].base != rsrc[2].limit ||
        rsrc[3].type != RT_IRQ || rsrc[3].base != rsrc[3].limit) {
        status = STATUS_INVALID_RESOURCE;
        goto has_error;
    }

    status = device_create(&dev, drv, parent);
    if (!CHECK_SUCCESS(status)) goto has_error;

    status = device_generate_name("ps2c", dev->name, sizeof(dev->name));
    if (!CHECK_SUCCESS(status)) goto has_error;

    data = malloc(sizeof(*data));
    data->io_data = rsrc[0].base;
    data->io_ctrl = rsrc[1].base;
    data->irq_port0 = rsrc[2].base;
    data->irq_port1 = rsrc[3].base;
    dev->data = data;

    /* disable all devices */
    io_out8(data->io_ctrl, 0xAD);
    io_out8(data->io_ctrl, 0xA7);
    
    /* flush the output buffer*/
    io_in8(data->io_data);

    /* disable translation & IRQ */
    io_out8(data->io_ctrl, 0x60);
    io_out8(data->io_data, 0x00);

    /* perform controller self-test */
    io_out8(data->io_ctrl, 0xAA);
    wait_output_buffer_full(dev, 5);
    if (io_in8(data->io_data) != 0x55) {
        return 1;
    }

    /* disable translation & IRQ (again) */
    io_out8(data->io_ctrl, 0x60);
    io_out8(data->io_data, 0x00);

    /* determine if the second port is available */
    int has_second_port = 0;
    io_out8(data->io_ctrl, 0xA8);
    io_out8(data->io_ctrl, 0x20);
    if (!(io_in8(data->io_data) & 0x20)) {
        has_second_port = 1;
        io_out8(data->io_ctrl, 0xA7);
    }

    /* initialize child devices */
    status = device_driver_find("ps2_keyboard", &kbdrv);
    if (!CHECK_SUCCESS(status)) {
        kbdrv = NULL;
    }

    status = device_driver_find("ps2_mouse", &msdrv);
    if (!CHECK_SUCCESS(status)) {
        msdrv = NULL;
    }
        
    if (kbdrv) {
        struct resource res[] = {
            {
                .type = RT_BUS,
                .base = 0,
                .limit = 0,
                .flags = 0,
            },
            {
                .type = RT_IRQ,
                .base = data->irq_port0,
                .limit = data->irq_port0,
                .flags = 0,
            },
        };

        status = kbdrv->probe(&idev, kbdrv, dev, res, ARRAY_SIZE(res));
        // ignore status
    }

    if (has_second_port && msdrv) {
        struct resource res[] = {
            {
                .type = RT_BUS,
                .base = 0,
                .limit = 0,
                .flags = 0,
            },
            {
                .type = RT_IRQ,
                .base = data->irq_port1,
                .limit = data->irq_port1,
                .flags = 0,
            },
        };

        status = kbdrv->probe(&idev, msdrv, dev, res, ARRAY_SIZE(res));
        // ignore status
    }
    
    if (devout) *devout = dev;

    return STATUS_SUCCESS;

has_error:
    if (dev) {
        for (struct device *partdev = dev->first_child; partdev; partdev = partdev->sibling) {
            partdev->driver->remove(partdev);
        }
    }

    if (data) {
        free(data);
    }

    if (dev) {
        device_remove(dev);
    }

    return status;
}

static status_t remove(struct device *dev)
{
    struct i8042_data *data = (struct i8042_data *)dev->data;

    for (struct device *partdev = dev->first_child; partdev; partdev = partdev->sibling) {
        partdev->driver->remove(partdev);
    }

    /* enable translation & IRQ */
    io_out8(data->io_ctrl, 0x60);
    io_out8(data->io_data, 0x63);

    free(data);

    device_remove(dev);

    return STATUS_SUCCESS;
}

static status_t get_interface(struct device *dev, const char *name, const void **result)
{
    if (strcmp(name, "ps2") == 0) {
        if (result) *result = &ps2if;
        return STATUS_SUCCESS;
    }

    return STATUS_ENTRY_NOT_FOUND;
}

DEVICE_DRIVER(i8042, i8042_init)
