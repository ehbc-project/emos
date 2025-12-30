#include <stdint.h>
#include <stdalign.h>

#include <endian.h>

#include <emos/types.h>
#include <emos/status.h>
#include <emos/memory.h>
#include <emos/object.h>
#include <emos/filesystem.h>
#include <emos/interface.h>
#include <emos/interface/char.h>
#include <emos/interface/wchar.h>
#include <emos/interface/block.h>
#include <emos/interface/filesystem.h>
#include <emos/log.h>
#include <emos/ioport.h>
#include <emos/uuid.h>
#include <emos/disk.h>

#include "fat.h"

static const struct filesystem_interface fsif;

static status_t early_deinit(struct filesystem_driver *drv);
static status_t deinit(struct filesystem_driver *drv);

int main(int argc, char **argv)
{
    status_t status;
    struct filesystem_driver *drv = NULL;

    emos_log(LOG_INFO, "init filesystem/fat\n");

    status = emos_filesystem_driver_create(&drv);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "emos_filesystem_driver_create() failed: 0x%08X\n", status);
        goto has_error;
    }

    drv->ops->early_deinit = early_deinit;
    drv->ops->deinit = deinit;
    drv->ops->probe = probe;
    drv->ops->mount = mount;
    drv->ops->unmount = unmount;

    status = emos_filesystem_driver_add_interface(drv, FILESYSTEM_INTERFACE_UUID, &fsif);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "emos_filesystem_driver_add_interface() failed: 0x%08X\n", status);
        goto has_error;
    }

    return STATUS_SUCCESS;

has_error:
    if (drv) {
        emos_filesystem_driver_remove(drv);
    }

    return status;
}

static status_t early_deinit(struct filesystem_driver *drv)
{
    return STATUS_SUCCESS;
}

static status_t deinit(struct filesystem_driver *drv)
{
    emos_filesystem_driver_remove(drv);

    return STATUS_SUCCESS;
}

static const struct filesystem_interface fsif = {
    .open = open,
    .close = close,
    .seek = seek,
    .read = read,
    .write = write,
    .tell = tell,
    .sync = sync,
    .flush = flush,
    .lock = lock,
    .unlock = unlock,
    .allocate = allocate,
    .truncate = truncate,

    .open_root_directory = open_root_directory,
    .open_directory = open_directory,
    .close_directory = close_directory,
    .create_file = create_file,
    .remove_file = remove_file,
    .create_directory = create_directory,
    .remove_directory = remove_directory,
    .move = move,
};
