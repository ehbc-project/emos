#include <string.h>
#include <stdio.h>

#include <mm/mm.h>
#include <bus/bus.h>
#include <device/driver.h>
#include <interface/floppy.h>
#include <asm/io.h>
#include <asm/isr.h>
#include <asm/time.h>
#include <asm/pause.h>

struct fdc_data {
    struct resource *res;
    uint8_t dor;
    volatile uint8_t irq_received;
    struct {
        uint8_t recalib, state, track;
    } drive_state[4];
};

#define FDCREG_SRA             0x0
#define FDCREG_SRB             0x1
#define FDCREG_DOR             0x2
#define FDCREG_TDR             0x3
#define FDCREG_MSR             0x4
#define FDCREG_DRSR            0x4
#define FDCREG_FIFO            0x5
#define FDCREG_DIR             0x7
#define FDCREG_CCR             0x7

static int wait_irq(struct device *dev, unsigned int timeout)
{
    struct fdc_data *data = (struct fdc_data *)dev->data;

    uint64_t tick_start = get_global_tick();

    if (data->irq_received) {
        data->irq_received = 0;
    }
    while (!data->irq_received) {
        if (get_global_tick() - tick_start > timeout) return 1;
    }
    data->irq_received = 0;

    return 0;
}

static int send_command(struct device *dev, struct fdc_command *cmd)
{
    struct fdc_data *data = (struct fdc_data *)dev->data;

    int err;
    uint8_t status;

    for (int i = 0; i < cmd->send_size + 1; i++) {
        while (!(_i686_in8(data->res->base + FDCREG_MSR) & 0x80)) {
            _i686_pause();
        }
        status = _i686_in8(data->res->base + FDCREG_MSR);
        if (status & 0x40) {
            return 1;
        }

        if (i > 0) {
            _i686_out8(data->res->base + FDCREG_FIFO, cmd->data[i - 1]);
        } else {
            _i686_out8(data->res->base + FDCREG_FIFO, cmd->data[0]);
        }
    }

    if (cmd->wait_irq) {
        err = wait_irq(dev, 1);
        if (err) return err;
    }

    for (int i = 0; i < cmd->recv_size; i++) {
        while (!(_i686_in8(data->res->base + FDCREG_MSR) & 0x80)) {
            _i686_pause();
        }
        status = _i686_in8(data->res->base + FDCREG_MSR);
        if (status & 0x40) {
            return 1;
        }

        cmd->data[i] = _i686_in8(data->res->base + FDCREG_FIFO);
    }

    status = _i686_in8(data->res->base + FDCREG_MSR);
    if (status & 0x40) {
        return 1;
    }

    return 0;
}

static int fdc_reset(struct device *dev)
{
    struct fdc_data *data = (struct fdc_data *)dev->data;

    for (int i = 0; i < 4; i++) {
        data->drive_state[i].recalib = 0;
        data->drive_state[i].state = 0;
        data->drive_state[i].track = 0;
    }

    struct fdc_command cmd;
    cmd.send_size = 3;
    cmd.recv_size = 0;
    cmd.wait_irq = 0;
    cmd.command = 0x13;
    cmd.data[0] = 0x00;
    cmd.data[0] = 0x20;
    cmd.data[0] = 0x00;
    send_command(dev, &cmd);

    return 0;
}

static int detect_device(struct device *dev, int slave, int *atapi)
{
    struct fdc_data *data = (struct fdc_data *)dev->data;

    return 0;
}

static int reset(struct device *dev, int drive)
{
    struct fdc_data *data = (struct fdc_data *)dev->data;

    data->drive_state[drive].recalib = 0;
    data->drive_state[drive].state = 0;
    data->drive_state[drive].track = 0;

    return 0;
}

struct floppy_interface flpif = {
    .reset = reset,
};

static void isr(struct device *dev, int num)
{
    
}

static int probe(struct device *dev);
static int remove(struct device *dev);
static const void *get_interface(struct device *dev, const char *name);

static struct device_driver drv = {
    .name = "fdc_isa",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static int probe(struct device *dev)
{
    if (!dev->resource || !dev->resource->next || !dev->resource->next->next) return 1;
    if (dev->resource->type != RT_IOPORT || dev->resource->next->type != RT_IRQ || dev->resource->next->next->type != RT_DMA) return 1;
    if (dev->resource->limit - dev->resource->base != 7 || dev->resource->next->base != dev->resource->next->limit || dev->resource->next->next->base != dev->resource->next->next->limit) return 1;

    dev->name = "fdc";
    dev->id = generate_device_id(dev->name);

    struct bus *floppy_bus = mm_allocate_clear(1, sizeof(*floppy_bus));
    floppy_bus->dev = dev;
    floppy_bus->name = "floppy";
    register_bus(floppy_bus);

    struct fdc_data *data = mm_allocate(sizeof(*data));
    data->res = dev->resource;
    data->dor = 0;
    for (int i = 0; i < 4; i++) {
        data->drive_state[i].recalib = 0;
        data->drive_state[i].state = 0;
        data->drive_state[i].track = 0;
    }

    dev->data = data;

    _pc_set_interrupt_handler(dev->resource->next->base, dev, isr);

    if (fdc_reset(dev)) return 1;

    for (int i = 0; i < 4; i++) {
        struct device *master_drive = mm_allocate_clear(1, sizeof(*master_drive));
        master_drive->bus = floppy_bus;
        master_drive->parent = dev;
        struct resource *res = create_resource(NULL);
        res->type = RT_BUS;
        res->base = i;
        res->limit = i;
        res->flags = 0;
        master_drive->resource = res;
        master_drive->driver = find_device_driver("floppy");
        register_device(master_drive);
    }

    return 0;
}

static int remove(struct device *dev)
{
    return 0;
}

static const void *get_interface(struct device *dev, const char *name)
{
    if (strcmp(name, "floppy") == 0) {
        return &flpif;
    }
    
    return NULL;
}

__attribute__((constructor))
static void _register_driver(void)
{
    register_device_driver(&drv);
}
