#include <stdio.h>

#include <device/device.h>
#include <device/driver.h>

int freopen_device(FILE *stream, const char *device_name)
{
    struct device *dev = find_device(device_name);
    if (!dev) return 1;
    stream->dev = dev;
    stream->charif = dev->driver->get_interface(dev, "char");
    if (!stream->charif) return 1;
    return 0;
}
