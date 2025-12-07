#include <stdint.h>
#include <stdalign.h>

#include <emos/types.h>
#include <emos/status.h>
#include <emos/memory.h>
#include <emos/object.h>
#include <emos/bus.h>
#include <emos/interface.h>
#include <emos/interface/char.h>
#include <emos/interface/wchar.h>
#include <emos/interface/block.h>
#include <emos/log.h>
#include <emos/ioport.h>
#include <emos/uuid.h>

static const struct char_interface charif;
static const struct wchar_interface wcharif;
static const struct block_interface blockif;

static status_t early_deinit(struct bus_driver *drv);
static status_t deinit(struct bus_driver *drv);

int main(int argc, char **argv)
{
    status_t status;
    struct bus_driver *drv = NULL;

    emos_log(LOG_INFO, "init bus/nonpnp\n");

    status = emos_bus_driver_create(&drv);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "emos_bus_driver_create() failed: 0x%08X\n", status);
        goto has_error;
    }

    drv->ops->early_deinit = early_deinit;
    drv->ops->deinit = deinit;

    status = emos_bus_driver_add_interface(drv, CHAR_INTERFACE_UUID, &charif);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "emos_bus_driver_add_interface() failed: 0x%08X\n", status);
        goto has_error;
    }
    
    status = emos_bus_driver_add_interface(drv, WCHAR_INTERFACE_UUID, &wcharif);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "emos_bus_driver_add_interface() failed: 0x%08X\n", status);
        goto has_error;
    }

    status = emos_bus_driver_add_interface(drv, BLOCK_INTERFACE_UUID, &blockif);
    if (!CHECK_SUCCESS(status)) {
        emos_log(LOG_ERROR, "emos_bus_driver_add_interface() failed: 0x%08X\n", status);
        goto has_error;
    }

    return STATUS_SUCCESS;

has_error:
    if (drv) {
        emos_bus_driver_remove(drv);
    }

    return status;
}

static status_t early_deinit(struct bus_driver *drv)
{
    return STATUS_SUCCESS;
}

static status_t deinit(struct bus_driver *drv)
{
    emos_bus_driver_remove(drv);

    return STATUS_SUCCESS;
}


