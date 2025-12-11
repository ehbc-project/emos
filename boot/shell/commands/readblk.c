#include <eboot/shell.h>

#include <stdio.h>
#include <stdlib.h>

#include <eboot/status.h>
#include <eboot/device.h>
#include <eboot/debug.h>
#include <eboot/interface/block.h>

static int readblk_handler(struct shell_instance *inst, int argc, char **argv)
{
    status_t status;
    lba_t lba;
    struct device *blkdev;
    const struct block_interface *blkif;
    uint8_t buf[512];

    if (argc < 3) {
        fprintf(stderr, "usage: %s device lba\n", argv[0]);
        return 1;
    }

    lba = strtoull(argv[2], NULL, 16);

    status = device_find(argv[1], &blkdev);
    if (!CHECK_SUCCESS(status)) {
        fprintf(stderr, "%s: could not find device\n", argv[0]);
        return 1;
    }
    status = blkdev->driver->get_interface(blkdev, "block", (const void **)&blkif);
    if (!CHECK_SUCCESS(status)) return 1;

    blkif->read(blkdev, lba, buf, 1, NULL);

    hexdump(stdout, buf, sizeof(buf), 0);

    return 0;
}

static struct command readblk_command = {
    .name = "readblk",
    .handler = readblk_handler,
    .help_message = "Read block from block device",
};

static void readblk_command_init(void)
{
    shell_command_register(&readblk_command);
}

SHELL_COMMAND(readblk, readblk_command_init)
