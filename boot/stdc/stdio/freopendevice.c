#include <stdio.h>

#include <device/device.h>
#include <device/driver.h>

int freopendevice(const char *__restrict device_name, FILE *__restrict stream)
{
    struct device *dev = find_device(device_name);
    if (!dev) return 1;
    stream->type = 2;
    stream->dev.dev = dev;
    stream->dev.charif = dev->driver->get_interface(dev, "char");
    if (!stream->dev.charif) return 1;
    return 0;
}
