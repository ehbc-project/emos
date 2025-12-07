#ifndef __EMOS_BUS_NONPNP_H__
#define __EMOS_BUS_NONPNP_H__

#include <emos/types.h>
#include <emos/device.h>

#define NONPNP_BUS_UUID UUID(0x60, 0x78, 0x63, 0x65, 0x68, 0x16, 0x5b, 0x44, 0x92, 0x66, 0x8E, 0x4D, 0x2D, 0x1A, 0x01, 0x65)

#define RT_MEMORY  0x00000001
#define RT_IOPORT  0x00000002
#define RT_IRQ     0x00000003
#define RT_DMA     0x00000004

struct nonpnp_resource {
    uint32_t type;
    uint32_t flags;
    uintptr_t start, end;
};

struct nonpnp_probe_args {
    int resource_count;
    struct nonpnp_resource resources[];
};

#endif // __EMOS_BUS_NONPNP_H__

