#include <stdio.h>

#include <device/device.h>
#include <device/driver.h>

int freopen_device(FILE *stream, const char *device_name)
{
    struct device *device = find_device(device_name);
    if (!device) return 1;
    stream->device = device;
    stream->char_if = device->driver->get_interface(device, "char");
    if (!stream->char_if) return 1;
    return 0;
}
