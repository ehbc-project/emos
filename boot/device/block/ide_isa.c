#include <string.h>
#include <stdio.h>

#include <mm/mm.h>
#include <bus/bus.h>
#include <device/driver.h>
#include <interface/ide.h>
#include <sys/io.h>
#include <asm/isr.h>
#include <asm/pause.h>

struct ide_data {
    struct bus *ide_bus;
    struct resource *res[2];
};

#define IDEREG_DATA     0
#define IDEREG_ERROR    1
#define IDEREG_FEATURES 1
#define IDEREG_COUNT    2
#define IDEREG_SECTOR   3
#define IDEREG_CYLLOW   4
#define IDEREG_CYLHIGH  5
#define IDEREG_DRVHEAD  6
#define IDEREG_STATUS   7
#define IDEREG_COMMAND  7

#define IDEREG_ALTSTAT  0
#define IDEREG_DEVCTRL  0
#define IDEREG_DRVADDR  1

static int bus_soft_reset(struct device *dev)
{
    struct ide_data *data = (struct ide_data *)dev->data;

    io_out8(data->res[1]->base + IDEREG_DEVCTRL, 0x04);
    io_in8(data->res[1]->base + IDEREG_ALTSTAT);
    io_out8(data->res[1]->base + IDEREG_DEVCTRL, 0x00);
    for (int i = 0; i < 20; i++) {
        io_in8(data->res[1]->base + IDEREG_ALTSTAT);
    }

    uint8_t status = io_in8(data->res[1]->base + IDEREG_ALTSTAT);
    if (status == 0xFF) {
        return 1;
    }
    do {
        _i686_pause();
        status = io_in8(data->res[1]->base + IDEREG_ALTSTAT);
    } while (status & 0x80);

    return 0;
}

static int send_command(struct device *dev, struct ata_command *cmd)
{
    struct ide_data *data = (struct ide_data *)dev->data;

    uint8_t status;
    do {
        _i686_pause();
        status = io_in8(data->res[0]->base + IDEREG_STATUS);
    } while (status & 0x80);

    if ((io_in8(data->res[0]->base + IDEREG_DRVHEAD) & 0x10) != (cmd->drive_head & 0x10)) {
        io_out8(data->res[0]->base + IDEREG_DRVHEAD, cmd->drive_head);
        io_in8(data->res[1]->base + IDEREG_ALTSTAT);
        io_in8(data->res[1]->base + IDEREG_ALTSTAT);
        io_in8(data->res[1]->base + IDEREG_ALTSTAT);
        io_in8(data->res[1]->base + IDEREG_ALTSTAT);
    }

    io_out8(data->res[0]->base + IDEREG_DRVHEAD, cmd->drive_head);

    if (cmd->extended) {
        io_out8(data->res[0]->base + IDEREG_FEATURES, (cmd->features >> 8) & 0xFF);
        io_out8(data->res[0]->base + IDEREG_COUNT, (cmd->count >> 8) & 0xFF);
        io_out8(data->res[0]->base + IDEREG_SECTOR, (cmd->sector >> 8) & 0xFF);
        io_out8(data->res[0]->base + IDEREG_CYLLOW, (cmd->cylinder_low >> 8) & 0xFF);
        io_out8(data->res[0]->base + IDEREG_CYLHIGH, (cmd->cylinder_high >> 8) & 0xFF);
    }

    io_out8(data->res[0]->base + IDEREG_FEATURES, cmd->features & 0xFF);
    io_out8(data->res[0]->base + IDEREG_COUNT, cmd->count & 0xFF);
    io_out8(data->res[0]->base + IDEREG_SECTOR, cmd->sector & 0xFF);
    io_out8(data->res[0]->base + IDEREG_CYLLOW, cmd->cylinder_low & 0xFF);
    io_out8(data->res[0]->base + IDEREG_CYLHIGH, cmd->cylinder_high & 0xFF);
    
    io_out8(data->res[0]->base + IDEREG_COMMAND, cmd->command);

    do {
        _i686_pause();
        status = io_in8(data->res[0]->base + IDEREG_STATUS);
    } while (status & 0x80);

    cmd->features = io_in8(data->res[0]->base + IDEREG_ERROR);
    cmd->count = io_in8(data->res[0]->base + IDEREG_COUNT);
    cmd->sector = io_in8(data->res[0]->base + IDEREG_SECTOR);
    cmd->cylinder_low = io_in8(data->res[0]->base + IDEREG_CYLLOW);
    cmd->cylinder_high = io_in8(data->res[0]->base + IDEREG_CYLHIGH);

    return status & 0x21;
}

static int send_command_pio_input(struct device *dev, struct ata_command *cmd, void *buf, long size, long count)
{
    struct ide_data *data = (struct ide_data *)dev->data;

    int err = send_command(dev, cmd);
    if (err) return err;

    uint8_t status;
    long transferred_count = 0;

    do {
        do {
            _i686_pause();
            status = io_in8(data->res[0]->base + IDEREG_STATUS);
            if (status & 0x21) goto end;
        } while (status & 0x80);
    
        io_ins16(data->res[0]->base + IDEREG_DATA, buf + transferred_count * size, size / sizeof(uint16_t));
    } while (++transferred_count < count);

end:
    return io_in8(data->res[0]->base + IDEREG_ERROR);
}

static int send_command_pio_output(struct device *dev, struct ata_command *cmd, const void *buf, long size, long count)
{
    struct ide_data *data = (struct ide_data *)dev->data;

    int err = send_command(dev, cmd);
    if (err) return err;

    uint8_t status;
    long transferred_count = 0;

    do {
        do {
            _i686_pause();
            status = io_in8(data->res[0]->base + IDEREG_STATUS);
            if (status & 0x01) goto end;
        } while (status & 0x08);
    
        io_outs16(data->res[0]->base + IDEREG_DATA, buf + transferred_count * size, size / sizeof(uint16_t));
    } while (++transferred_count < count);

end:
    return io_in8(data->res[0]->base + IDEREG_ERROR);
}

const struct ide_interface ideif = {
    .send_command = send_command,
    .send_command_pio_input = send_command_pio_input,
    .send_command_pio_output = send_command_pio_output,
};

static int detect_device(struct device *dev, int slave, int *atapi)
{
    struct ide_data *data = (struct ide_data *)dev->data;

    io_out8(data->res[0]->base + IDEREG_DRVHEAD, 0xA0 | (slave ? 0x10 : 0x00));
    io_in8(data->res[1]->base + IDEREG_ALTSTAT);
    io_in8(data->res[1]->base + IDEREG_ALTSTAT);
    io_in8(data->res[1]->base + IDEREG_ALTSTAT);
    io_in8(data->res[1]->base + IDEREG_ALTSTAT);
    if (io_in8(data->res[1]->base + IDEREG_ALTSTAT) == 0x00) return 1;

    uint16_t idbuf[256];
    struct atapi_device_ident *atapi_ident = (struct atapi_device_ident *)&idbuf;
    
    struct ata_command cmd = {
        .extended = 0,
        .command = 0xEC,  /* IDENTIFY DEVICE */
        .features = 0,
        .count = 0,
        .sector = 0,
        .cylinder_low = 0,
        .cylinder_high = 0,
        .drive_head = 0xA0 | (slave ? 0x10 : 0x00),
    };
    
    if (send_command_pio_input(dev, &cmd, idbuf, sizeof(idbuf), 1)) {
        if (cmd.cylinder_low != 0x14 || cmd.cylinder_high != 0xEB) {
            return 1;
        }
    }

    if (cmd.cylinder_low != 0x14 || cmd.cylinder_high != 0xEB) {
        if (atapi) *atapi = 0;
        return 0;
    }

    cmd = (struct ata_command){
        .extended = 0,
        .command = 0xA1,  /* IDENTIFY PACKET DEVICE */
        .features = 0,
        .count = 0,
        .sector = 0,
        .cylinder_low = 0,
        .cylinder_high = 0,
        .drive_head = 0xA0 | (slave ? 0x10 : 0x00),
    };

    if (send_command_pio_input(dev, &cmd, idbuf, sizeof(idbuf), 1)) {
        return 1;
    }

    if (atapi) *atapi = 1;
    return 0;
}

static void isr(struct device *dev, int num)
{
    struct ide_data *data = (struct ide_data *)dev->data;
}

static int probe(struct device *dev);
static int remove(struct device *dev);
static const void *get_interface(struct device *dev, const char *name);

static struct device_driver drv = {
    .name = "ide_isa",
    .probe = probe,
    .remove = remove,
    .get_interface = get_interface,
};

static int probe(struct device *dev)
{
    struct resource *res[2];
    if (!dev->resource || !dev->resource->next) return 1;
    res[0] = dev->resource;
    res[1] = dev->resource->next;
    if (res[0]->type != RT_IOPORT || res[1]->type != RT_IOPORT || dev->resource->next->next->type != RT_IRQ) return 1;
    if (res[0]->limit - res[0]->base != 7 || res[1]->limit - res[1]->base != 1 || dev->resource->next->next->limit != dev->resource->next->next->base) {
        return 1;
    }

    dev->name = "idectrl";
    dev->id = generate_device_id(dev->name);

    struct bus *ide_bus = mm_allocate_clear(1, sizeof(*ide_bus));
    ide_bus->dev = dev;
    ide_bus->name = "ide";
    register_bus(ide_bus);

    struct ide_data *data = mm_allocate(sizeof(*data));
    data->ide_bus = ide_bus;
    data->res[0] = res[0];
    data->res[1] = res[1];

    dev->data = data;

    if (bus_soft_reset(dev)) return 1;
    
    _pc_set_interrupt_handler(dev->resource->next->next->base, dev, isr);

    int master_atapi, master_present = !detect_device(dev, 0, &master_atapi);
    int slave_atapi, slave_present = !detect_device(dev, 1, &slave_atapi);
    if (!master_present && !slave_present) return 1;

    if (master_present) {
        struct device *master_drive = mm_allocate_clear(1, sizeof(*master_drive));
        master_drive->bus = ide_bus;
        master_drive->parent = dev;
        struct resource *res = create_resource(NULL);
        res->type = RT_BUS;
        res->base = res->limit = 0;
        res->flags = 0;
        master_drive->resource = res;
        master_drive->driver = find_device_driver(master_atapi ? "atapi" : "ata");
        register_device(master_drive);
    }

    if (slave_present) {
        struct device *slave_drive = mm_allocate_clear(1, sizeof(*slave_drive));
        slave_drive->bus = ide_bus;
        slave_drive->parent = dev;
        struct resource *res = create_resource(NULL);
        res->type = RT_BUS;
        res->base = res->limit = 1;
        res->flags = 0;
        slave_drive->resource = res;
        slave_drive->driver = find_device_driver(slave_atapi ? "atapi" : "ata");
        register_device(slave_drive);    
    }

    return 0;
}

static int remove(struct device *dev)
{
    struct ide_data *data = (struct ide_data *)dev->data;

    mm_free(data);

    return 0;
}

static const void *get_interface(struct device *dev, const char *name)
{
    if (strcmp(name, "ide") == 0) {
        return &ideif;
    }
    
    return NULL;
}

DEVICE_DRIVER(drv)
